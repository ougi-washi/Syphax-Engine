// Syphax-Engine - Ougi Washi

#include "se_debug.h"
#include "se_text.h"
#include "syphax/s_thread.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef s_array(se_debug_trace_event, se_debug_trace_events);
typedef s_array(se_debug_trace_stat, se_debug_trace_stats_array);

static se_debug_level g_debug_level = SE_DEBUG_LEVEL_INFO;
static u32 g_debug_category_mask = SE_DEBUG_CATEGORY_ALL;
static se_debug_log_callback g_log_callback = NULL;
static void* g_log_user_data = NULL;
static b8 g_overlay_enabled = false;
static se_debug_trace_events g_trace_events = {0};
static se_text_handle* g_overlay_text_handle = NULL;
static se_context* g_overlay_context = NULL;
static se_debug_frame_timing g_frame_timing_current = {0};
static se_debug_frame_timing g_frame_timing_last = {0};
static b8 g_frame_active = false;
static f64 g_frame_begin_time = 0.0;
static u64 g_frame_counter = 0;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_trace_mutex = PTHREAD_MUTEX_INITIALIZER;

static f64 se_debug_now_seconds(void) {
	struct timespec ts = {0};
	timespec_get(&ts, TIME_UTC);
	return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
}

static u64 se_debug_thread_id_to_u64(const s_thread_id thread_id) {
	u64 value = 0u;
	const sz copy_bytes = sizeof(thread_id) < sizeof(value) ? sizeof(thread_id) : sizeof(value);
	memcpy(&value, &thread_id, copy_bytes);
	return value;
}

static u64 se_debug_current_thread_id_u64(void) {
	return se_debug_thread_id_to_u64(s_thread_current_id());
}

static void se_debug_format_timestamp(c8* out_timestamp, const sz out_timestamp_size) {
	if (!out_timestamp || out_timestamp_size == 0) {
		return;
	}
	out_timestamp[0] = '\0';
	const time_t now_seconds = time(NULL);
	struct tm local_time = {0};
#if defined(_WIN32)
	if (localtime_s(&local_time, &now_seconds) != 0) {
		snprintf(out_timestamp, out_timestamp_size, "0000-00-00|00:00:00");
		return;
	}
#else
	if (localtime_r(&now_seconds, &local_time) == NULL) {
		snprintf(out_timestamp, out_timestamp_size, "0000-00-00|00:00:00");
		return;
	}
#endif
	if (strftime(out_timestamp, out_timestamp_size, "%Y-%m-%d|%H:%M:%S", &local_time) == 0) {
		snprintf(out_timestamp, out_timestamp_size, "0000-00-00|00:00:00");
	}
}

static void se_debug_write_line(FILE* stream, const c8* line) {
	if (!stream || !line) {
		return;
	}
	const sz line_size = strlen(line);
	if (line_size > 0) {
		fwrite(line, 1, line_size, stream);
	}
	fputc('\n', stream);
	fflush(stream);
}

static void se_debug_buffer_append(c8* out_buffer, const sz out_buffer_size, sz* cursor, const c8* fmt, ...) {
	if (!out_buffer || out_buffer_size == 0 || !cursor || !fmt) {
		return;
	}
	if (*cursor >= out_buffer_size) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	const i32 written = vsnprintf(out_buffer + *cursor, out_buffer_size - *cursor, fmt, args);
	va_end(args);
	if (written < 0) {
		return;
	}
	const sz written_size = (sz)written;
	if (written_size >= out_buffer_size - *cursor) {
		*cursor = out_buffer_size;
		return;
	}
	*cursor += written_size;
}

static void se_debug_trace_events_init(void) {
	if (s_array_get_capacity(&g_trace_events) == 0) {
		s_array_init(&g_trace_events);
		s_array_reserve(&g_trace_events, 8192);
	}
}

static void se_debug_frame_timing_reset(se_debug_frame_timing* timing) {
	if (!timing) {
		return;
	}
	memset(timing, 0, sizeof(*timing));
}

static void se_debug_frame_timing_add_duration_locked(const c8* name, const f64 duration_seconds) {
	if (!g_frame_active || !name || duration_seconds <= 0.0) {
		return;
	}
	const f64 duration_ms = duration_seconds * 1000.0;
	if (strcmp(name, "window_update") == 0) {
		g_frame_timing_current.window_update_ms += duration_ms;
		return;
	}
	if (strcmp(name, "input_tick") == 0) {
		g_frame_timing_current.input_ms += duration_ms;
		return;
	}
	if (strcmp(name, "scene2d_render") == 0) {
		g_frame_timing_current.scene2d_ms += duration_ms;
		return;
	}
	if (strcmp(name, "scene3d_render") == 0) {
		g_frame_timing_current.scene3d_ms += duration_ms;
		return;
	}
	if (strcmp(name, "text_render") == 0) {
		g_frame_timing_current.text_ms += duration_ms;
		return;
	}
	if (strcmp(name, "ui_render") == 0) {
		g_frame_timing_current.ui_ms += duration_ms;
		return;
	}
	if (strcmp(name, "navigation_astar") == 0) {
		g_frame_timing_current.navigation_ms += duration_ms;
		return;
	}
	if (strcmp(name, "window_present") == 0) {
		g_frame_timing_current.window_present_ms += duration_ms;
		return;
	}
	g_frame_timing_current.other_ms += duration_ms;
}

static void se_debug_frame_timing_finalize_locked(const f64 now_seconds) {
	if (!g_frame_active) {
		return;
	}
	g_frame_timing_current.frame_ms = s_max(0.0, (now_seconds - g_frame_begin_time) * 1000.0);
	const f64 accounted_ms =
		g_frame_timing_current.window_update_ms +
		g_frame_timing_current.input_ms +
		g_frame_timing_current.scene2d_ms +
		g_frame_timing_current.scene3d_ms +
		g_frame_timing_current.text_ms +
		g_frame_timing_current.ui_ms +
		g_frame_timing_current.navigation_ms +
		g_frame_timing_current.window_present_ms +
		g_frame_timing_current.other_ms;
	if (g_frame_timing_current.frame_ms > accounted_ms) {
		g_frame_timing_current.other_ms += (g_frame_timing_current.frame_ms - accounted_ms);
	}
	g_frame_timing_current.frame_index = ++g_frame_counter;
	g_frame_timing_last = g_frame_timing_current;
	se_debug_frame_timing_reset(&g_frame_timing_current);
	g_frame_active = false;
	g_frame_begin_time = 0.0;
}

static u32 se_debug_trace_depth_locked(const u64 thread_id, const se_debug_trace_channel channel) {
	u32 depth = 0u;
	for (sz i = 0; i < s_array_get_size(&g_trace_events); ++i) {
		const se_debug_trace_event* event = s_array_get(&g_trace_events, s_array_handle(&g_trace_events, (u32)i));
		if (!event || !event->active) {
			continue;
		}
		if (event->thread_id == thread_id && event->channel == (u32)channel) {
			depth++;
		}
	}
	return depth;
}

static i32 se_debug_trace_stat_compare_desc(const void* left, const void* right) {
	const se_debug_trace_stat* a = (const se_debug_trace_stat*)left;
	const se_debug_trace_stat* b = (const se_debug_trace_stat*)right;
	if (a->total_ms < b->total_ms) {
		return 1;
	}
	if (a->total_ms > b->total_ms) {
		return -1;
	}
	if (a->max_ms < b->max_ms) {
		return 1;
	}
	if (a->max_ms > b->max_ms) {
		return -1;
	}
	return strcmp(a->name, b->name);
}

static se_debug_trace_stat* se_debug_trace_stat_find(
	se_debug_trace_stats_array* stats,
	const se_debug_trace_event* event) {
	if (!stats || !event) {
		return NULL;
	}
	for (sz i = 0; i < s_array_get_size(stats); ++i) {
		se_debug_trace_stat* stat = s_array_get(stats, s_array_handle(stats, (u32)i));
		if (!stat) {
			continue;
		}
		if (stat->thread_id != event->thread_id ||
			stat->channel != event->channel ||
			strcmp(stat->name, event->name) != 0) {
			continue;
		}
		return stat;
	}
	return NULL;
}

static void se_debug_trace_stats_finalize(
	se_debug_trace_stats_array* stats,
	const b8 last_frame_only,
	const se_debug_frame_timing* frame_timing) {
	if (!stats || !frame_timing) {
		return;
	}
	const f64 frame_ms = (last_frame_only && frame_timing->frame_ms > 0.0) ? frame_timing->frame_ms : 0.0;
	for (sz i = 0; i < s_array_get_size(stats); ++i) {
		se_debug_trace_stat* stat = s_array_get(stats, s_array_handle(stats, (u32)i));
		if (!stat) {
			continue;
		}
		if (stat->call_count > 0u) {
			stat->avg_ms = stat->total_ms / (f64)stat->call_count;
		}
		if (frame_ms > 0.0) {
			stat->frame_percent = (stat->total_ms / frame_ms) * 100.0;
		} else {
			stat->frame_percent = 0.0;
		}
	}
}

static void se_debug_log_default(const se_debug_level level, const se_debug_category category, const c8* message, void* user_data) {
	(void)level;
	(void)category;
	(void)user_data;
	c8 timestamp[32] = {0};
	c8 line[1152] = {0};
	se_debug_format_timestamp(timestamp, sizeof(timestamp));
	snprintf(line, sizeof(line), "[%s] %s", timestamp, message ? message : "");
	se_debug_write_line(stderr, line);
}

void se_debug_set_level(const se_debug_level level) {
	pthread_mutex_lock(&g_log_mutex);
	g_debug_level = level;
	pthread_mutex_unlock(&g_log_mutex);
}

se_debug_level se_debug_get_level(void) {
	pthread_mutex_lock(&g_log_mutex);
	const se_debug_level level = g_debug_level;
	pthread_mutex_unlock(&g_log_mutex);
	return level;
}

void se_debug_set_category_mask(const u32 category_mask) {
	pthread_mutex_lock(&g_log_mutex);
	g_debug_category_mask = category_mask;
	pthread_mutex_unlock(&g_log_mutex);
}

u32 se_debug_get_category_mask(void) {
	pthread_mutex_lock(&g_log_mutex);
	const u32 category_mask = g_debug_category_mask;
	pthread_mutex_unlock(&g_log_mutex);
	return category_mask;
}

void se_debug_set_log_callback(se_debug_log_callback callback, void* user_data) {
	pthread_mutex_lock(&g_log_mutex);
	g_log_callback = callback;
	g_log_user_data = user_data;
	pthread_mutex_unlock(&g_log_mutex);
}

void se_log(const c8* fmt, ...) {
	if (!fmt) {
		return;
	}
	c8 message[1024] = {0};
	c8 timestamp[32] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);
	se_debug_format_timestamp(timestamp, sizeof(timestamp));
	c8 line[1152] = {0};
	snprintf(line, sizeof(line), "[%s] %s", timestamp, message);

	pthread_mutex_lock(&g_log_mutex);
	se_debug_write_line(stdout, line);
	pthread_mutex_unlock(&g_log_mutex);
}

void se_debug_log(const se_debug_level level, const se_debug_category category, const c8* fmt, ...) {
	if (!fmt) {
		return;
	}
	c8 message[1024] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);

	se_debug_log_callback callback = NULL;
	void* callback_user_data = NULL;
	pthread_mutex_lock(&g_log_mutex);
	if (level < g_debug_level || ((u32)category & g_debug_category_mask) == 0u) {
		pthread_mutex_unlock(&g_log_mutex);
		return;
	}
	callback = g_log_callback;
	callback_user_data = g_log_user_data;
	if (callback == NULL) {
		se_debug_log_default(level, category, message, NULL);
		pthread_mutex_unlock(&g_log_mutex);
		return;
	}
	pthread_mutex_unlock(&g_log_mutex);
	callback(level, category, message, callback_user_data);
}

b8 se_debug_validate(const b8 condition, const se_result error_code, const c8* expression, const c8* file, const i32 line) {
	if (condition) {
		return true;
	}
	se_set_last_error(error_code);
	se_debug_log(
		SE_DEBUG_LEVEL_ERROR,
		SE_DEBUG_CATEGORY_CORE,
		"se_debug_validate :: validation failed: `%s` at %s:%d -> %s",
		expression,
		file,
		line,
		se_result_str(error_code));
	return false;
}

const c8* se_debug_trace_channel_name(const se_debug_trace_channel channel) {
	switch (channel) {
		case SE_DEBUG_TRACE_CHANNEL_DEFAULT:
			return "default";
		case SE_DEBUG_TRACE_CHANNEL_CPU:
			return "cpu";
		case SE_DEBUG_TRACE_CHANNEL_GPU:
			return "gpu";
		default:
			return "custom";
	}
}

void se_debug_trace_begin_channel(const c8* name, const se_debug_trace_channel channel) {
	if (!name || name[0] == '\0') {
		return;
	}
	const f64 now = se_debug_now_seconds();
	const u64 thread_id = se_debug_current_thread_id_u64();

	pthread_mutex_lock(&g_trace_mutex);
	se_debug_trace_events_init();
	se_debug_trace_event event = {0};
	strncpy(event.name, name, sizeof(event.name) - 1);
	event.begin_time = now;
	event.end_time = 0.0;
	event.duration = 0.0;
	event.frame_index = g_frame_active ? (g_frame_counter + 1u) : g_frame_counter;
	event.thread_id = thread_id;
	event.channel = (u32)channel;
	event.depth = se_debug_trace_depth_locked(thread_id, channel);
	event.active = true;
	s_array_add(&g_trace_events, event);
	pthread_mutex_unlock(&g_trace_mutex);
}

void se_debug_trace_end_channel(const c8* name, const se_debug_trace_channel channel) {
	if (!name || name[0] == '\0') {
		return;
	}
	const f64 now = se_debug_now_seconds();
	const u64 thread_id = se_debug_current_thread_id_u64();

	pthread_mutex_lock(&g_trace_mutex);
	if (s_array_get_size(&g_trace_events) == 0) {
		pthread_mutex_unlock(&g_trace_mutex);
		return;
	}
	for (sz i = s_array_get_size(&g_trace_events); i > 0; --i) {
		se_debug_trace_event* event = s_array_get(&g_trace_events, s_array_handle(&g_trace_events, (u32)(i - 1)));
		if (!event || !event->active) {
			continue;
		}
		if (event->thread_id != thread_id ||
			event->channel != (u32)channel ||
			strcmp(event->name, name) != 0) {
			continue;
		}
		event->end_time = now;
		event->duration = s_max(0.0, event->end_time - event->begin_time);
		event->active = false;
		if (channel == SE_DEBUG_TRACE_CHANNEL_DEFAULT || channel == SE_DEBUG_TRACE_CHANNEL_CPU) {
			se_debug_frame_timing_add_duration_locked(name, event->duration);
		}
		pthread_mutex_unlock(&g_trace_mutex);
		return;
	}
	pthread_mutex_unlock(&g_trace_mutex);
}

void se_debug_trace_begin(const c8* name) {
	se_debug_trace_begin_channel(name, SE_DEBUG_TRACE_CHANNEL_CPU);
}

void se_debug_trace_end(const c8* name) {
	se_debug_trace_end_channel(name, SE_DEBUG_TRACE_CHANNEL_CPU);
}

b8 se_debug_get_trace_events(const se_debug_trace_event** out_events, sz* out_count) {
	if (!out_events || !out_count) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	pthread_mutex_lock(&g_trace_mutex);
	se_debug_trace_events_init();
	*out_events = s_array_get_data(&g_trace_events);
	*out_count = s_array_get_size(&g_trace_events);
	pthread_mutex_unlock(&g_trace_mutex);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_debug_get_trace_stats(se_debug_trace_stat* out_stats, u32 max_stats, u32* out_count, b8 last_frame_only) {
	if (!out_count || (max_stats > 0u && !out_stats)) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	se_debug_trace_stats_array stats = {0};
	s_array_init(&stats);

	pthread_mutex_lock(&g_trace_mutex);
	se_debug_trace_events_init();
	const u64 frame_filter = last_frame_only ? g_frame_timing_last.frame_index : 0u;
	for (sz i = 0; i < s_array_get_size(&g_trace_events); ++i) {
		const se_debug_trace_event* event = s_array_get(&g_trace_events, s_array_handle(&g_trace_events, (u32)i));
		if (!event || event->active || event->duration <= 0.0) {
			continue;
		}
		if (last_frame_only && event->frame_index != frame_filter) {
			continue;
		}

		se_debug_trace_stat* stat = se_debug_trace_stat_find(&stats, event);
		if (!stat) {
			se_debug_trace_stat new_stat = {0};
			strncpy(new_stat.name, event->name, sizeof(new_stat.name) - 1);
			new_stat.frame_index = frame_filter;
			new_stat.thread_id = event->thread_id;
			new_stat.channel = event->channel;
			s_array_add(&stats, new_stat);
			stat = se_debug_trace_stat_find(&stats, event);
			if (!stat) {
				continue;
			}
		}

		const f64 duration_ms = event->duration * 1000.0;
		stat->call_count++;
		stat->total_ms += duration_ms;
		if (duration_ms > stat->max_ms) {
			stat->max_ms = duration_ms;
		}
	}

	se_debug_trace_stats_finalize(&stats, last_frame_only, &g_frame_timing_last);
	if (s_array_get_size(&stats) > 1) {
		qsort(s_array_get_data(&stats), s_array_get_size(&stats), sizeof(se_debug_trace_stat), se_debug_trace_stat_compare_desc);
	}
	pthread_mutex_unlock(&g_trace_mutex);

	const u32 stats_count = (u32)s_array_get_size(&stats);
	const u32 copy_count = max_stats < stats_count ? max_stats : stats_count;
	if (copy_count > 0u && out_stats) {
		const se_debug_trace_stat* data = s_array_get_data(&stats);
		memcpy(out_stats, data, sizeof(*out_stats) * copy_count);
	}
	*out_count = stats_count;
	s_array_clear(&stats);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_debug_dump_trace_stats(c8* out_buffer, sz out_buffer_size, u32 max_entries, b8 last_frame_only) {
	if (!out_buffer || out_buffer_size == 0) {
		return;
	}
	out_buffer[0] = '\0';
	if (max_entries == 0u) {
		max_entries = 10u;
	}
	if (max_entries > 256u) {
		max_entries = 256u;
	}

	se_debug_trace_stat* stats = (se_debug_trace_stat*)calloc(max_entries, sizeof(*stats));
	if (!stats) {
		snprintf(out_buffer, out_buffer_size, "trace_stats: out-of-memory");
		return;
	}

	u32 total_count = 0u;
	if (!se_debug_get_trace_stats(stats, max_entries, &total_count, last_frame_only)) {
		snprintf(out_buffer, out_buffer_size, "trace_stats: unavailable");
		free(stats);
		return;
	}

	se_debug_frame_timing frame_timing = {0};
	(void)se_debug_get_last_frame_timing(&frame_timing);

	sz cursor = 0u;
	se_debug_buffer_append(
		out_buffer,
		out_buffer_size,
		&cursor,
		"trace_stats mode=%s shown=%u total=%u frame=%llu\n",
		last_frame_only ? "last_frame" : "all_frames",
		max_entries,
		total_count,
		(unsigned long long)frame_timing.frame_index);

	for (u32 i = 0u; i < max_entries; ++i) {
		const se_debug_trace_stat* stat = &stats[i];
		if (stat->call_count == 0u) {
			break;
		}
		se_debug_buffer_append(
			out_buffer,
			out_buffer_size,
			&cursor,
			"%02u thread=0x%llx channel=%s name=%s calls=%llu total=%.3fms avg=%.3fms max=%.3fms",
			i + 1u,
			(unsigned long long)stat->thread_id,
			se_debug_trace_channel_name((se_debug_trace_channel)stat->channel),
			stat->name,
			(unsigned long long)stat->call_count,
			stat->total_ms,
			stat->avg_ms,
			stat->max_ms);
		if (last_frame_only) {
			se_debug_buffer_append(
				out_buffer,
				out_buffer_size,
				&cursor,
				" frame_pct=%.2f%%",
				stat->frame_percent);
		}
		se_debug_buffer_append(out_buffer, out_buffer_size, &cursor, "\n");
	}

	free(stats);
}

void se_debug_clear_trace_events(void) {
	pthread_mutex_lock(&g_trace_mutex);
	se_debug_trace_events_init();
	while (s_array_get_size(&g_trace_events) > 0) {
		s_array_remove(&g_trace_events, s_array_handle(&g_trace_events, (u32)(s_array_get_size(&g_trace_events) - 1)));
	}
	pthread_mutex_unlock(&g_trace_mutex);
}

void se_debug_frame_begin(void) {
	const f64 now = se_debug_now_seconds();
	pthread_mutex_lock(&g_trace_mutex);
	if (g_frame_active) {
		se_debug_frame_timing_finalize_locked(now);
	}
	se_debug_frame_timing_reset(&g_frame_timing_current);
	g_frame_active = true;
	g_frame_begin_time = now;
	pthread_mutex_unlock(&g_trace_mutex);
}

void se_debug_frame_end(void) {
	const f64 now = se_debug_now_seconds();
	pthread_mutex_lock(&g_trace_mutex);
	se_debug_frame_timing_finalize_locked(now);
	pthread_mutex_unlock(&g_trace_mutex);
}

b8 se_debug_get_last_frame_timing(se_debug_frame_timing* out_timing) {
	if (!out_timing) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	pthread_mutex_lock(&g_trace_mutex);
	*out_timing = g_frame_timing_last;
	pthread_mutex_unlock(&g_trace_mutex);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_debug_dump_last_frame_timing(c8* out_buffer, const sz out_buffer_size) {
	if (!out_buffer || out_buffer_size == 0) {
		return;
	}
	se_debug_frame_timing timing = {0};
	(void)se_debug_get_last_frame_timing(&timing);
	snprintf(
		out_buffer,
		out_buffer_size,
		"frame=%llu ms=%.3f win_upd=%.3f input=%.3f scene2d=%.3f scene3d=%.3f text=%.3f ui=%.3f nav=%.3f present=%.3f other=%.3f",
		(unsigned long long)timing.frame_index,
		timing.frame_ms,
		timing.window_update_ms,
		timing.input_ms,
		timing.scene2d_ms,
		timing.scene3d_ms,
		timing.text_ms,
		timing.ui_ms,
		timing.navigation_ms,
		timing.window_present_ms,
		timing.other_ms);
}

void se_debug_dump_last_frame_timing_lines(c8* out_buffer, const sz out_buffer_size) {
	if (!out_buffer || out_buffer_size == 0) {
		return;
	}
	se_debug_frame_timing timing = {0};
	(void)se_debug_get_last_frame_timing(&timing);
	snprintf(
		out_buffer,
		out_buffer_size,
		"frame=%llu total=%.3fms\n"
		"window_update=%.3fms input=%.3fms\n"
		"scene2d=%.3fms scene3d=%.3fms\n"
		"text=%.3fms ui=%.3fms\n"
		"navigation=%.3fms present=%.3fms\n"
		"other=%.3fms",
		(unsigned long long)timing.frame_index,
		timing.frame_ms,
		timing.window_update_ms,
		timing.input_ms,
		timing.scene2d_ms,
		timing.scene3d_ms,
		timing.text_ms,
		timing.ui_ms,
		timing.navigation_ms,
		timing.window_present_ms,
		timing.other_ms);
}

b8 se_debug_collect_stats(const se_window_handle window, se_input_handle* input, se_debug_system_stats* out_stats) {
	if (!out_stats) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	memset(out_stats, 0, sizeof(*out_stats));
	se_context* ctx = se_current_context();
	if (!ctx) {
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (window != S_HANDLE_NULL) {
		se_window_get_diagnostics(window, &out_stats->window);
	}
	if (input) {
		se_input_get_diagnostics(input, &out_stats->input);
	}
	out_stats->window_count = (u32)s_array_get_size(&ctx->windows);
	out_stats->camera_count = (u32)s_array_get_size(&ctx->cameras);
	out_stats->scene2d_count = (u32)s_array_get_size(&ctx->scenes_2d);
	out_stats->scene3d_count = (u32)s_array_get_size(&ctx->scenes_3d);
	out_stats->object2d_count = (u32)s_array_get_size(&ctx->objects_2d);
	out_stats->object3d_count = (u32)s_array_get_size(&ctx->objects_3d);
	out_stats->ui_element_count = (u32)s_array_get_size(&ctx->ui_elements);
	pthread_mutex_lock(&g_trace_mutex);
	out_stats->navigation_trace_count = (u32)s_array_get_size(&g_trace_events);
	pthread_mutex_unlock(&g_trace_mutex);
	se_set_last_error(SE_RESULT_OK);
	return true;
}

void se_debug_set_overlay_enabled(const b8 enabled) {
	g_overlay_enabled = enabled;
}

b8 se_debug_is_overlay_enabled(void) {
	return g_overlay_enabled;
}

void se_debug_render_overlay(const se_window_handle window, se_input_handle* input) {
	if (!g_overlay_enabled || window == S_HANDLE_NULL) {
		return;
	}
	se_context* ctx = se_current_context();
	if (!ctx) {
		return;
	}

	if (g_overlay_context != ctx) {
		if (g_overlay_text_handle) {
			se_text_handle_destroy(g_overlay_text_handle);
		}
		g_overlay_text_handle = se_text_handle_create(0);
		g_overlay_context = ctx;
	}
	if (!g_overlay_text_handle) {
		return;
	}

	se_debug_system_stats stats = {0};
	if (!se_debug_collect_stats(window, input, &stats)) {
		return;
	}

	se_font_handle font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 16.0f);
	if (font == S_HANDLE_NULL) {
		return;
	}

	c8 timing_text[512] = {0};
	se_debug_dump_last_frame_timing_lines(timing_text, sizeof(timing_text));

	c8 text[1536] = {0};
	snprintf(
		text,
		sizeof(text),
		"dbg win:%u cam:%u sc2:%u sc3:%u obj2:%u obj3:%u ui:%u fps:%u traces:%u\n%s",
		stats.window_count,
		stats.camera_count,
		stats.scene2d_count,
		stats.scene3d_count,
		stats.object2d_count,
		stats.object3d_count,
		stats.ui_element_count,
		(u32)(stats.window.last_present_duration > 0.0 ? (1.0 / stats.window.last_present_duration) : 0.0),
		stats.navigation_trace_count,
		timing_text);
	se_text_render(g_overlay_text_handle, font, text, &s_vec2(-0.98f, 0.96f), &s_vec2(1.0f, 1.0f), 0.017f);
}
