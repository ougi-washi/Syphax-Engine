// Syphax-Engine - Ougi Washi

#include "render/se_render_queue.h"

#include "se_graphics.h"
#include "syphax/s_thread.h"
#include "window/se_window_backend_internal.h"

#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

#define SE_RENDER_QUEUE_PACKET_COUNT 2u
#define SE_RENDER_QUEUE_ALIGN_BYTES ((u32)sizeof(void*))
#define SE_RENDER_QUEUE_DEFAULT_MAX_COMMANDS 4096u
#define SE_RENDER_QUEUE_DEFAULT_MAX_COMMAND_BYTES (4u * 1024u * 1024u)

typedef struct {
	se_render_queue_sync_fn fn;
	u32 payload_offset;
	u32 payload_bytes;
} se_render_queue_command;

typedef struct {
	se_render_queue_command* commands;
	u8* payload_bytes;
	u32 command_capacity;
	u32 payload_capacity;
	u32 command_count;
	u32 payload_used;
	b8 submitted;
} se_render_queue_packet;

typedef struct {
	s_mutex mutex;
	s_cond work_ready;
	s_cond started;
	s_cond sync_done;
	s_cond present_done;
	s_thread thread;

	b8 initialized;
	b8 thread_started;
	b8 started_signal;
	b8 running;
	b8 stopping;
	b8 failed;
	b8 frame_open;
	b8 wait_on_submit;

	se_window_handle window;
	se_context* context;
	s_thread_id render_thread_id;

	se_render_queue_sync_fn sync_fn;
	const void* sync_payload;
	void* sync_out_result;
	b8 sync_pending;
	b8 sync_completed;

	b8 present_pending;
	i32 pending_packet_index;
	u32 record_packet_index;
	se_render_queue_packet packets[SE_RENDER_QUEUE_PACKET_COUNT];

	u32 max_commands_per_frame;
	u32 max_command_bytes_per_frame;

	u64 submitted_frames;
	u64 presented_frames;
	u64 submit_stalls;
	u64 last_command_count;
	u64 last_command_bytes;
	u64 current_command_count;
	u64 current_command_bytes;
	f64 last_submit_wait_ms;
	f64 last_execute_ms;
	f64 last_present_ms;
} se_render_queue_runtime;

static se_render_queue_runtime g_render_queue = {0};

static f64 se_render_queue_now_seconds(void) {
#if defined(_WIN32)
	static LARGE_INTEGER frequency = {0};
	if (frequency.QuadPart == 0) {
		QueryPerformanceFrequency(&frequency);
	}
	LARGE_INTEGER value = {0};
	QueryPerformanceCounter(&value);
	if (frequency.QuadPart <= 0) {
		return 0.0;
	}
	return (f64)value.QuadPart / (f64)frequency.QuadPart;
#else
	struct timespec ts = {0};
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
#endif
}

static u32 se_render_queue_align_up(const u32 value, const u32 alignment) {
	if (alignment <= 1u) {
		return value;
	}
	const u32 mask = alignment - 1u;
	return (value + mask) & ~mask;
}

static u32 se_render_queue_depth_locked(const se_render_queue_runtime* runtime) {
	if (!runtime || runtime->submitted_frames <= runtime->presented_frames) {
		return 0u;
	}
	u64 depth = runtime->submitted_frames - runtime->presented_frames;
	if (depth > 0xFFFFFFFFu) {
		depth = 0xFFFFFFFFu;
	}
	return (u32)depth;
}

static void se_render_queue_packet_reset(se_render_queue_packet* packet) {
	if (!packet) {
		return;
	}
	packet->command_count = 0u;
	packet->payload_used = 0u;
}

static void se_render_queue_release_packets(se_render_queue_runtime* runtime) {
	if (!runtime) {
		return;
	}
	for (u32 i = 0u; i < SE_RENDER_QUEUE_PACKET_COUNT; ++i) {
		se_render_queue_packet* packet = &runtime->packets[i];
		if (packet->commands) {
			free(packet->commands);
		}
		if (packet->payload_bytes) {
			free(packet->payload_bytes);
		}
		memset(packet, 0, sizeof(*packet));
	}
}

static b8 se_render_queue_allocate_packets(se_render_queue_runtime* runtime) {
	if (!runtime) {
		return false;
	}

	se_render_queue_release_packets(runtime);

	const u32 command_capacity = runtime->max_commands_per_frame > 0u ?
		runtime->max_commands_per_frame : SE_RENDER_QUEUE_DEFAULT_MAX_COMMANDS;
	const u32 payload_capacity = runtime->max_command_bytes_per_frame > 0u ?
		runtime->max_command_bytes_per_frame : SE_RENDER_QUEUE_DEFAULT_MAX_COMMAND_BYTES;
	if (command_capacity == 0u || payload_capacity == 0u) {
		return false;
	}

	for (u32 i = 0u; i < SE_RENDER_QUEUE_PACKET_COUNT; ++i) {
		se_render_queue_packet* packet = &runtime->packets[i];
		packet->commands = (se_render_queue_command*)malloc(sizeof(se_render_queue_command) * (sz)command_capacity);
		packet->payload_bytes = (u8*)malloc((sz)payload_capacity);
		if (!packet->commands || !packet->payload_bytes) {
			se_render_queue_release_packets(runtime);
			return false;
		}
		packet->command_capacity = command_capacity;
		packet->payload_capacity = payload_capacity;
		packet->submitted = false;
		se_render_queue_packet_reset(packet);
	}

	return true;
}

static b8 se_render_queue_record_locked(se_render_queue_runtime* runtime,
	const se_render_queue_sync_fn fn,
	const void* payload,
	const u32 payload_bytes,
	const u32 pointer_patch_offset,
	const void* blob,
	const u32 blob_bytes) {
	if (!runtime || !fn) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (payload_bytes > 0u && payload == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (blob_bytes > 0u && blob == NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (pointer_patch_offset != SE_RENDER_QUEUE_POINTER_PATCH_NONE &&
		(pointer_patch_offset + (u32)sizeof(void*) > payload_bytes || blob_bytes == 0u)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	if (!runtime->running || runtime->failed || runtime->stopping) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	if (!runtime->frame_open) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}

	if (runtime->record_packet_index >= SE_RENDER_QUEUE_PACKET_COUNT) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	se_render_queue_packet* packet = &runtime->packets[runtime->record_packet_index];
	if (!packet->commands || !packet->payload_bytes || packet->submitted) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	if (packet->command_count >= packet->command_capacity) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	const u32 aligned_offset = se_render_queue_align_up(packet->payload_used, SE_RENDER_QUEUE_ALIGN_BYTES);
	const u32 total_payload = payload_bytes + blob_bytes;
	if (aligned_offset > packet->payload_capacity || total_payload > (packet->payload_capacity - aligned_offset)) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	se_render_queue_command* command = &packet->commands[packet->command_count];
	command->fn = fn;
	command->payload_offset = aligned_offset;
	command->payload_bytes = total_payload;

	u8* payload_dst = packet->payload_bytes + aligned_offset;
	if (payload_bytes > 0u) {
		memcpy(payload_dst, payload, payload_bytes);
	}
	if (blob_bytes > 0u) {
		u8* blob_dst = payload_dst + payload_bytes;
		memcpy(blob_dst, blob, blob_bytes);
		if (pointer_patch_offset != SE_RENDER_QUEUE_POINTER_PATCH_NONE) {
			void** patch_ptr = (void**)(payload_dst + pointer_patch_offset);
			*patch_ptr = blob_dst;
		}
	}

	packet->command_count++;
	packet->payload_used = aligned_offset + total_payload;
	runtime->current_command_count = packet->command_count;
	runtime->current_command_bytes = packet->payload_used;
	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void se_render_queue_execute_packet(const se_render_queue_packet* packet) {
	if (!packet || !packet->commands || !packet->payload_bytes || packet->command_count == 0u) {
		return;
	}
	for (u32 i = 0u; i < packet->command_count; ++i) {
		const se_render_queue_command* command = &packet->commands[i];
		if (!command->fn) {
			continue;
		}
		const void* command_payload = packet->payload_bytes + command->payload_offset;
		command->fn(command_payload, NULL);
	}
}

typedef struct {
	se_render_queue_packet* packet;
} se_render_queue_flush_payload;

static void se_render_queue_flush_packet_exec(const void* payload, void* out_result) {
	(void)out_result;
	const se_render_queue_flush_payload* args = (const se_render_queue_flush_payload*)payload;
	if (!args || !args->packet) {
		return;
	}
	se_render_queue_execute_packet(args->packet);
}

static b8 se_render_queue_dispatch_sync_locked(se_render_queue_runtime* runtime,
	const se_render_queue_sync_fn fn,
	const void* payload,
	void* out_result) {
	if (!runtime || !fn) {
		return false;
	}

	runtime->sync_fn = fn;
	runtime->sync_payload = payload;
	runtime->sync_out_result = out_result;
	runtime->sync_completed = false;
	runtime->sync_pending = true;
	s_cond_signal(&runtime->work_ready);

	while (!runtime->sync_completed && runtime->running && !runtime->failed) {
		s_cond_wait(&runtime->sync_done, &runtime->mutex);
	}
	const b8 ok = runtime->running && !runtime->failed;
	runtime->sync_fn = NULL;
	runtime->sync_payload = NULL;
	runtime->sync_out_result = NULL;
	return ok;
}

static void* se_render_queue_thread_main(void* user_data) {
	se_render_queue_runtime* runtime = (se_render_queue_runtime*)user_data;
	if (!runtime) {
		return NULL;
	}

	se_context* previous_context = se_push_tls_context(runtime->context);
	const b8 attached = se_window_backend_render_thread_attach(runtime->window);
	const b8 render_ready = attached ? se_render_init() : false;

	s_mutex_lock(&runtime->mutex);
	runtime->started_signal = true;
	if (!attached || !render_ready) {
		runtime->failed = true;
		runtime->running = false;
		runtime->stopping = true;
		s_cond_broadcast(&runtime->started);
		s_mutex_unlock(&runtime->mutex);
		se_pop_tls_context(previous_context);
		return NULL;
	}
	runtime->render_thread_id = s_thread_current_id();
	runtime->running = true;
	s_cond_broadcast(&runtime->started);
	s_mutex_unlock(&runtime->mutex);

	for (;;) {
		se_render_queue_sync_fn sync_fn = NULL;
		const void* sync_payload = NULL;
		void* sync_out_result = NULL;
		i32 packet_index = -1;
		b8 do_present = false;

		s_mutex_lock(&runtime->mutex);
		while (!runtime->stopping && !runtime->sync_pending && runtime->pending_packet_index < 0 && !runtime->present_pending) {
			s_cond_wait(&runtime->work_ready, &runtime->mutex);
		}
		if (runtime->sync_pending) {
			sync_fn = runtime->sync_fn;
			sync_payload = runtime->sync_payload;
			sync_out_result = runtime->sync_out_result;
			runtime->sync_pending = false;
			runtime->sync_completed = false;
		} else if (runtime->pending_packet_index >= 0 || runtime->present_pending) {
			packet_index = runtime->pending_packet_index;
			runtime->pending_packet_index = -1;
			do_present = runtime->present_pending;
			runtime->present_pending = false;
		} else if (runtime->stopping) {
			s_mutex_unlock(&runtime->mutex);
			break;
		}
		s_mutex_unlock(&runtime->mutex);

		if (sync_fn) {
			const f64 execute_begin = se_render_queue_now_seconds();
			sync_fn(sync_payload, sync_out_result);
			const f64 execute_end = se_render_queue_now_seconds();

			s_mutex_lock(&runtime->mutex);
			runtime->last_execute_ms = (execute_end - execute_begin) * 1000.0;
			runtime->sync_completed = true;
			s_cond_broadcast(&runtime->sync_done);
			s_mutex_unlock(&runtime->mutex);
			continue;
		}

		if (packet_index >= 0) {
			const f64 execute_begin = se_render_queue_now_seconds();
			se_render_queue_execute_packet(&runtime->packets[(u32)packet_index]);
			const f64 execute_end = se_render_queue_now_seconds();
			s_mutex_lock(&runtime->mutex);
			runtime->last_execute_ms = (execute_end - execute_begin) * 1000.0;
			s_mutex_unlock(&runtime->mutex);
		}

		if (do_present) {
			const f64 present_begin = se_render_queue_now_seconds();
			se_window_backend_render_thread_present(runtime->window);
			const f64 present_end = se_render_queue_now_seconds();

			s_mutex_lock(&runtime->mutex);
			runtime->presented_frames++;
			runtime->last_present_ms = (present_end - present_begin) * 1000.0;
			if (packet_index >= 0 && (u32)packet_index < SE_RENDER_QUEUE_PACKET_COUNT) {
				runtime->packets[(u32)packet_index].submitted = false;
			}
			s_cond_broadcast(&runtime->present_done);
			s_mutex_unlock(&runtime->mutex);
			continue;
		}
	}

	se_window_backend_render_thread_detach();
	se_pop_tls_context(previous_context);

	s_mutex_lock(&runtime->mutex);
	runtime->running = false;
	runtime->sync_completed = true;
	runtime->present_pending = false;
	runtime->pending_packet_index = -1;
	for (u32 i = 0u; i < SE_RENDER_QUEUE_PACKET_COUNT; ++i) {
		runtime->packets[i].submitted = false;
	}
	s_cond_broadcast(&runtime->sync_done);
	s_cond_broadcast(&runtime->present_done);
	s_mutex_unlock(&runtime->mutex);
	return NULL;
}

static void se_render_queue_reset_for_start(se_render_queue_runtime* runtime,
	const se_window_handle window,
	const se_render_thread_config* config) {
	if (!runtime) {
		return;
	}
	const se_render_thread_config defaults = SE_RENDER_THREAD_CONFIG_DEFAULTS;
	const se_render_thread_config cfg = config ? *config : defaults;
	runtime->window = window;
	runtime->context = se_current_context();
	runtime->running = false;
	runtime->stopping = false;
	runtime->failed = false;
	runtime->frame_open = false;
	runtime->wait_on_submit = cfg.wait_on_submit;
	runtime->max_commands_per_frame = cfg.max_commands_per_frame;
	runtime->max_command_bytes_per_frame = cfg.max_command_bytes_per_frame;
	runtime->submitted_frames = 0u;
	runtime->presented_frames = 0u;
	runtime->submit_stalls = 0u;
	runtime->last_command_count = 0u;
	runtime->last_command_bytes = 0u;
	runtime->current_command_count = 0u;
	runtime->current_command_bytes = 0u;
	runtime->last_submit_wait_ms = 0.0;
	runtime->last_execute_ms = 0.0;
	runtime->last_present_ms = 0.0;
	runtime->sync_fn = NULL;
	runtime->sync_payload = NULL;
	runtime->sync_out_result = NULL;
	runtime->sync_pending = false;
	runtime->sync_completed = true;
	runtime->present_pending = false;
	runtime->pending_packet_index = -1;
	runtime->record_packet_index = 0u;
	runtime->started_signal = false;
	for (u32 i = 0u; i < SE_RENDER_QUEUE_PACKET_COUNT; ++i) {
		runtime->packets[i].submitted = false;
		se_render_queue_packet_reset(&runtime->packets[i]);
	}
	memset(&runtime->render_thread_id, 0, sizeof(runtime->render_thread_id));
}

b8 se_render_queue_start(const se_window_handle window, const se_render_thread_config* config) {
	if (window == S_HANDLE_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		if (!s_mutex_init(&runtime->mutex)) {
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		if (!s_cond_init(&runtime->work_ready)) {
			s_mutex_destroy(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		if (!s_cond_init(&runtime->started)) {
			s_cond_destroy(&runtime->work_ready);
			s_mutex_destroy(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		if (!s_cond_init(&runtime->sync_done)) {
			s_cond_destroy(&runtime->started);
			s_cond_destroy(&runtime->work_ready);
			s_mutex_destroy(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		if (!s_cond_init(&runtime->present_done)) {
			s_cond_destroy(&runtime->sync_done);
			s_cond_destroy(&runtime->started);
			s_cond_destroy(&runtime->work_ready);
			s_mutex_destroy(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
		runtime->initialized = true;
	}

	s_mutex_lock(&runtime->mutex);
	if (runtime->thread_started || runtime->running) {
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	se_render_queue_reset_for_start(runtime, window, config);
	if (!se_render_queue_allocate_packets(runtime)) {
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	runtime->thread_started = true;
	if (!s_thread_create(&runtime->thread, se_render_queue_thread_main, runtime)) {
		runtime->thread_started = false;
		se_render_queue_release_packets(runtime);
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}
	while (!runtime->started_signal) {
		s_cond_wait(&runtime->started, &runtime->mutex);
	}
	const b8 ok = runtime->running && !runtime->failed;
	s_mutex_unlock(&runtime->mutex);

	if (!ok) {
		(void)s_thread_join(&runtime->thread, NULL);
		s_mutex_lock(&runtime->mutex);
		runtime->thread_started = false;
		runtime->window = S_HANDLE_NULL;
		runtime->context = NULL;
		se_render_queue_release_packets(runtime);
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}

	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_render_queue_stop(const se_window_handle window) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		return;
	}

	s_mutex_lock(&runtime->mutex);
	if (!runtime->thread_started || runtime->window != window) {
		s_mutex_unlock(&runtime->mutex);
		return;
	}
	runtime->stopping = true;
	s_cond_broadcast(&runtime->work_ready);
	s_cond_broadcast(&runtime->sync_done);
	s_cond_broadcast(&runtime->present_done);
	s_mutex_unlock(&runtime->mutex);

	(void)s_thread_join(&runtime->thread, NULL);

	s_mutex_lock(&runtime->mutex);
	runtime->thread_started = false;
	runtime->window = S_HANDLE_NULL;
	runtime->context = NULL;
	runtime->frame_open = false;
	runtime->pending_packet_index = -1;
	se_render_queue_release_packets(runtime);
	s_mutex_unlock(&runtime->mutex);
}

b8 se_render_queue_is_running_for_window(const se_window_handle window) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		return false;
	}
	s_mutex_lock(&runtime->mutex);
	const b8 running = runtime->running && runtime->window == window && !runtime->failed;
	s_mutex_unlock(&runtime->mutex);
	return running;
}

b8 se_render_queue_is_running(void) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		return false;
	}
	s_mutex_lock(&runtime->mutex);
	const b8 running = runtime->running && !runtime->failed;
	s_mutex_unlock(&runtime->mutex);
	return running;
}

b8 se_render_queue_is_render_thread(void) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		return false;
	}
	s_mutex_lock(&runtime->mutex);
	const b8 running = runtime->running;
	const s_thread_id render_thread_id = runtime->render_thread_id;
	s_mutex_unlock(&runtime->mutex);
	if (!running) {
		return false;
	}
	return s_thread_id_equal(s_thread_current_id(), render_thread_id);
}

se_window_handle se_render_queue_active_window(void) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		return S_HANDLE_NULL;
	}
	s_mutex_lock(&runtime->mutex);
	const se_window_handle window = runtime->running ? runtime->window : S_HANDLE_NULL;
	s_mutex_unlock(&runtime->mutex);
	return window;
}

void se_render_queue_wait_idle(const se_window_handle window) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		return;
	}

	s_mutex_lock(&runtime->mutex);
	if (!runtime->running || runtime->window != window) {
		s_mutex_unlock(&runtime->mutex);
		return;
	}
	while (!runtime->sync_completed ||
		runtime->sync_pending ||
		runtime->submitted_frames > runtime->presented_frames ||
		runtime->present_pending ||
		runtime->pending_packet_index >= 0) {
		if (!runtime->sync_completed || runtime->sync_pending) {
			s_cond_wait(&runtime->sync_done, &runtime->mutex);
		} else {
			s_cond_wait(&runtime->present_done, &runtime->mutex);
		}
	}
	s_mutex_unlock(&runtime->mutex);
}

b8 se_render_queue_get_diagnostics(const se_window_handle window, se_render_thread_diagnostics* out_diag) {
	if (!out_diag) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		memset(out_diag, 0, sizeof(*out_diag));
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	s_mutex_lock(&runtime->mutex);
	if (runtime->window != window || !runtime->thread_started) {
		s_mutex_unlock(&runtime->mutex);
		memset(out_diag, 0, sizeof(*out_diag));
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	out_diag->running = runtime->running;
	out_diag->stopping = runtime->stopping;
	out_diag->failed = runtime->failed;
	out_diag->submitted_frames = runtime->submitted_frames;
	out_diag->presented_frames = runtime->presented_frames;
	out_diag->submit_stalls = runtime->submit_stalls;
	out_diag->queue_depth = se_render_queue_depth_locked(runtime);
	out_diag->last_command_count = runtime->last_command_count;
	out_diag->last_command_bytes = runtime->last_command_bytes;
	out_diag->last_submit_wait_ms = runtime->last_submit_wait_ms;
	out_diag->last_execute_ms = runtime->last_execute_ms;
	out_diag->last_present_ms = runtime->last_present_ms;
	s_mutex_unlock(&runtime->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_render_queue_begin_frame(const se_window_handle window) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return;
	}

	s_mutex_lock(&runtime->mutex);
	if (!runtime->running || runtime->window != window || runtime->failed || runtime->frame_open) {
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return;
	}

	u32 packet_index = runtime->record_packet_index;
	if (packet_index >= SE_RENDER_QUEUE_PACKET_COUNT) {
		packet_index = 0u;
	}
	if (runtime->packets[packet_index].submitted) {
		packet_index = (packet_index + 1u) % SE_RENDER_QUEUE_PACKET_COUNT;
		while (runtime->packets[packet_index].submitted && runtime->running && !runtime->failed) {
			s_cond_wait(&runtime->present_done, &runtime->mutex);
		}
		if (!runtime->running || runtime->failed) {
			s_mutex_unlock(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return;
		}
	}

	runtime->record_packet_index = packet_index;
	se_render_queue_packet_reset(&runtime->packets[packet_index]);
	runtime->frame_open = true;
	runtime->current_command_count = 0u;
	runtime->current_command_bytes = 0u;
	s_mutex_unlock(&runtime->mutex);
	se_set_last_error(SE_RESULT_OK);
}

void se_render_queue_submit_frame(const se_window_handle window) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return;
	}

	const f64 wait_begin = se_render_queue_now_seconds();
	f64 wait_end = wait_begin;
	s_mutex_lock(&runtime->mutex);
	if (!runtime->running || runtime->window != window || runtime->failed || !runtime->frame_open) {
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return;
	}

	while (runtime->submitted_frames > runtime->presented_frames) {
		runtime->submit_stalls++;
		s_cond_wait(&runtime->present_done, &runtime->mutex);
		if (!runtime->running || runtime->failed) {
			s_mutex_unlock(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return;
		}
	}
	wait_end = se_render_queue_now_seconds();
	runtime->last_submit_wait_ms = (wait_end - wait_begin) * 1000.0;

	const u32 packet_index = runtime->record_packet_index;
	if (packet_index >= SE_RENDER_QUEUE_PACKET_COUNT) {
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return;
	}
	se_render_queue_packet* packet = &runtime->packets[packet_index];
	if (packet->submitted) {
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return;
	}

	runtime->frame_open = false;
	packet->submitted = true;
	runtime->submitted_frames++;
	runtime->last_command_count = packet->command_count;
	runtime->last_command_bytes = packet->payload_used;
	runtime->pending_packet_index = (i32)packet_index;
	runtime->present_pending = true;
	s_cond_signal(&runtime->work_ready);

	if (runtime->wait_on_submit) {
		while (runtime->submitted_frames > runtime->presented_frames) {
			s_cond_wait(&runtime->present_done, &runtime->mutex);
		}
	}
	s_mutex_unlock(&runtime->mutex);
	se_set_last_error(SE_RESULT_OK);
}

void se_render_queue_wait_presented(const se_window_handle window) {
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		return;
	}
	s_mutex_lock(&runtime->mutex);
	if (!runtime->running || runtime->window != window) {
		s_mutex_unlock(&runtime->mutex);
		return;
	}
	while (runtime->submitted_frames > runtime->presented_frames) {
		s_cond_wait(&runtime->present_done, &runtime->mutex);
	}
	s_mutex_unlock(&runtime->mutex);
}

b8 se_render_queue_get_frame_stats(const se_window_handle window, se_render_frame_stats* out_stats) {
	if (!out_stats) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		memset(out_stats, 0, sizeof(*out_stats));
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	s_mutex_lock(&runtime->mutex);
	if (!runtime->running || runtime->window != window) {
		s_mutex_unlock(&runtime->mutex);
		memset(out_stats, 0, sizeof(*out_stats));
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	out_stats->submitted_frames = runtime->submitted_frames;
	out_stats->presented_frames = runtime->presented_frames;
	out_stats->submit_stalls = runtime->submit_stalls;
	out_stats->queue_depth = se_render_queue_depth_locked(runtime);
	out_stats->last_command_count = runtime->last_command_count;
	out_stats->last_command_bytes = runtime->last_command_bytes;
	out_stats->last_submit_wait_ms = runtime->last_submit_wait_ms;
	out_stats->last_execute_ms = runtime->last_execute_ms;
	out_stats->last_present_ms = runtime->last_present_ms;
	s_mutex_unlock(&runtime->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_render_queue_call_sync_sized(const se_render_queue_sync_fn fn, const void* payload, void* out_result, const u32 payload_bytes) {
	if (!fn) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	if (se_render_queue_is_render_thread()) {
		fn(payload, out_result);
		se_set_last_error(SE_RESULT_OK);
		return true;
	}

	s_mutex_lock(&runtime->mutex);
	if (!runtime->running || runtime->failed || runtime->stopping) {
		s_mutex_unlock(&runtime->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}

	while (runtime->submitted_frames > runtime->presented_frames || runtime->present_pending) {
		s_cond_wait(&runtime->present_done, &runtime->mutex);
		if (!runtime->running || runtime->failed || runtime->stopping) {
			s_mutex_unlock(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
	}

	while (runtime->sync_pending) {
		s_cond_wait(&runtime->sync_done, &runtime->mutex);
		if (!runtime->running || runtime->failed || runtime->stopping) {
			s_mutex_unlock(&runtime->mutex);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return false;
		}
	}

	if (runtime->frame_open && runtime->record_packet_index < SE_RENDER_QUEUE_PACKET_COUNT) {
		se_render_queue_packet* packet = &runtime->packets[runtime->record_packet_index];
		if (packet->command_count > 0u) {
			const se_render_queue_flush_payload flush_payload = {
				.packet = packet
			};
			if (!se_render_queue_dispatch_sync_locked(runtime, se_render_queue_flush_packet_exec, &flush_payload, NULL)) {
				s_mutex_unlock(&runtime->mutex);
				se_set_last_error(SE_RESULT_BACKEND_FAILURE);
				return false;
			}
			se_render_queue_packet_reset(packet);
			runtime->current_command_count = 0u;
			runtime->current_command_bytes = 0u;
		}
		runtime->current_command_count++;
		runtime->current_command_bytes += payload_bytes;
	}

	const b8 ok = se_render_queue_dispatch_sync_locked(runtime, fn, payload, out_result);
	s_mutex_unlock(&runtime->mutex);

	se_set_last_error(ok ? SE_RESULT_OK : SE_RESULT_BACKEND_FAILURE);
	return ok;
}

b8 se_render_queue_call_sync(const se_render_queue_sync_fn fn, const void* payload, void* out_result) {
	return se_render_queue_call_sync_sized(fn, payload, out_result, 0u);
}

b8 se_render_queue_record_async_blob(const se_render_queue_sync_fn fn,
	const void* payload,
	u32 payload_bytes,
	u32 pointer_patch_offset,
	const void* blob,
	u32 blob_bytes) {
	if (!fn) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_render_queue_runtime* runtime = &g_render_queue;
	if (!runtime->initialized) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}
	if (se_render_queue_is_render_thread()) {
		if (payload_bytes > 0u && payload == NULL) {
			se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
			return false;
		}
		fn(payload, NULL);
		se_set_last_error(SE_RESULT_OK);
		return true;
	}

	s_mutex_lock(&runtime->mutex);
	const b8 ok = se_render_queue_record_locked(runtime,
		fn,
		payload,
		payload_bytes,
		pointer_patch_offset,
		blob,
		blob_bytes);
	s_mutex_unlock(&runtime->mutex);
	return ok;
}

b8 se_render_queue_record_async(const se_render_queue_sync_fn fn, const void* payload, const u32 payload_bytes) {
	return se_render_queue_record_async_blob(fn,
		payload,
		payload_bytes,
		SE_RENDER_QUEUE_POINTER_PATCH_NONE,
		NULL,
		0u);
}
