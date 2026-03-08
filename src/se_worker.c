// Syphax-Engine - Ougi Washi

#include "se_worker.h"

#include "syphax/s_thread.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
	SE_WORKER_TASK_STATE_QUEUED = 0,
	SE_WORKER_TASK_STATE_RUNNING,
	SE_WORKER_TASK_STATE_DONE
} se_worker_task_state;

typedef enum {
	SE_WORKER_SUBMIT_OK = 0,
	SE_WORKER_SUBMIT_INVALID,
	SE_WORKER_SUBMIT_STOPPING,
	SE_WORKER_SUBMIT_TASKS_FULL,
	SE_WORKER_SUBMIT_QUEUE_FULL
} se_worker_submit_result;

typedef struct {
	se_worker_task_fn fn;
	void* user_data;
	void* result;
	se_worker_task_state state;
} se_worker_task_record;

typedef struct {
	se_worker_pool* pool;
	u32 index;
} se_worker_thread_context;

typedef struct {
	se_worker_task task;
	se_worker_task_fn fn;
	void* user_data;
	b8 valid;
} se_worker_task_claim;

typedef struct {
	se_worker_for_fn fn;
	void* user_data;
	u32 count;
	u32 batch_size;
	u32 next_index;
	s_mutex mutex;
} se_worker_parallel_for_state;

typedef s_array(se_worker_task_record, se_worker_task_records);

struct se_worker_pool {
	se_worker_config config;
	se_worker_task_records tasks;
	s_thread* threads;
	se_worker_thread_context* thread_contexts;
	se_worker_task* queue;
	s_mutex mutex;
	s_cond work_ready;
	s_cond state_changed;
	u64 submitted_tasks;
	u64 completed_tasks;
	u32 active_tasks;
	u32 queue_head;
	u32 queue_count;
	u32 started_threads;
	b8 mutex_ready : 1;
	b8 work_ready_ready : 1;
	b8 state_changed_ready : 1;
	b8 running : 1;
	b8 stopping : 1;
};

static SE_THREAD_LOCAL se_worker_pool* g_se_worker_tls_pool = NULL;
static SE_THREAD_LOCAL se_worker_task g_se_worker_tls_task = SE_WORKER_TASK_NULL;

static se_worker_config se_worker_resolve_config(const se_worker_config* config);
static se_result se_worker_submit_result_to_error(se_worker_submit_result result);
static se_worker_submit_result se_worker_submit_locked(se_worker_pool* pool, se_worker_task_fn fn, void* user_data, se_worker_task* out_task);
static b8 se_worker_try_claim_task_locked(se_worker_pool* pool, se_worker_task_claim* out_claim);
static void se_worker_execute_claim(se_worker_pool* pool, const se_worker_task_claim* claim);
static void* se_worker_thread_main(void* user_data);
static void* se_worker_parallel_for_task(void* user_data);
static b8 se_worker_wait_for_queue_room(se_worker_pool* pool);
static void se_worker_shutdown_threads(se_worker_pool* pool);
static void se_worker_cleanup_pool(se_worker_pool* pool);

static se_worker_config se_worker_resolve_config(const se_worker_config* config) {
	const se_worker_config defaults = SE_WORKER_CONFIG_DEFAULTS;
	se_worker_config resolved = defaults;
	if (config) {
		resolved = *config;
		if (resolved.thread_count == 0u) {
			resolved.thread_count = defaults.thread_count;
		}
		if (resolved.queue_capacity == 0u) {
			resolved.queue_capacity = defaults.queue_capacity;
		}
		if (resolved.max_tasks == 0u) {
			resolved.max_tasks = defaults.max_tasks;
		}
	}
	return resolved;
}

static se_result se_worker_submit_result_to_error(const se_worker_submit_result result) {
	switch (result) {
		case SE_WORKER_SUBMIT_OK: return SE_RESULT_OK;
		case SE_WORKER_SUBMIT_INVALID: return SE_RESULT_INVALID_ARGUMENT;
		case SE_WORKER_SUBMIT_STOPPING: return SE_RESULT_UNSUPPORTED;
		case SE_WORKER_SUBMIT_TASKS_FULL:
		case SE_WORKER_SUBMIT_QUEUE_FULL: return SE_RESULT_CAPACITY_EXCEEDED;
	}
	return SE_RESULT_BACKEND_FAILURE;
}

static se_worker_submit_result se_worker_submit_locked(
	se_worker_pool* pool,
	se_worker_task_fn fn,
	void* user_data,
	se_worker_task* out_task) {
	if (!pool || !fn || !out_task) {
		return SE_WORKER_SUBMIT_INVALID;
	}
	if (!pool->running || pool->stopping) {
		return SE_WORKER_SUBMIT_STOPPING;
	}
	if (s_array_get_size(&pool->tasks) >= pool->config.max_tasks) {
		return SE_WORKER_SUBMIT_TASKS_FULL;
	}
	if (pool->queue_count >= pool->config.queue_capacity) {
		return SE_WORKER_SUBMIT_QUEUE_FULL;
	}

	se_worker_task_record record = {0};
	record.fn = fn;
	record.user_data = user_data;
	record.state = SE_WORKER_TASK_STATE_QUEUED;

	const se_worker_task task = s_array_add(&pool->tasks, record);
	const u32 tail = (pool->queue_head + pool->queue_count) % pool->config.queue_capacity;
	pool->queue[tail] = task;
	pool->queue_count++;
	pool->submitted_tasks++;
	*out_task = task;

	s_cond_signal(&pool->work_ready);
	s_cond_broadcast(&pool->state_changed);
	return SE_WORKER_SUBMIT_OK;
}

static b8 se_worker_try_claim_task_locked(se_worker_pool* pool, se_worker_task_claim* out_claim) {
	if (!pool || !out_claim) {
		return false;
	}

	memset(out_claim, 0, sizeof(*out_claim));
	while (pool->queue_count > 0u) {
		const se_worker_task task = pool->queue[pool->queue_head];
		pool->queue_head = (pool->queue_head + 1u) % pool->config.queue_capacity;
		pool->queue_count--;

		se_worker_task_record* record = s_array_get(&pool->tasks, task);
		if (!record) {
			s_cond_broadcast(&pool->state_changed);
			continue;
		}

		record->state = SE_WORKER_TASK_STATE_RUNNING;
		pool->active_tasks++;

		out_claim->task = task;
		out_claim->fn = record->fn;
		out_claim->user_data = record->user_data;
		out_claim->valid = true;

		s_cond_broadcast(&pool->state_changed);
		return true;
	}

	return false;
}

static void se_worker_execute_claim(se_worker_pool* pool, const se_worker_task_claim* claim) {
	if (!pool || !claim || !claim->valid || !claim->fn) {
		return;
	}

	se_worker_pool* previous_pool = g_se_worker_tls_pool;
	const se_worker_task previous_task = g_se_worker_tls_task;
	g_se_worker_tls_pool = pool;
	g_se_worker_tls_task = claim->task;
	void* result = claim->fn(claim->user_data);
	g_se_worker_tls_pool = previous_pool;
	g_se_worker_tls_task = previous_task;

	s_mutex_lock(&pool->mutex);
	se_worker_task_record* record = s_array_get(&pool->tasks, claim->task);
	if (record) {
		record->result = result;
		record->state = SE_WORKER_TASK_STATE_DONE;
	}
	if (pool->active_tasks > 0u) {
		pool->active_tasks--;
	}
	pool->completed_tasks++;
	s_cond_broadcast(&pool->state_changed);
	s_mutex_unlock(&pool->mutex);
}

static void* se_worker_thread_main(void* user_data) {
	se_worker_thread_context* context = (se_worker_thread_context*)user_data;
	if (!context || !context->pool) {
		return NULL;
	}

	se_worker_pool* pool = context->pool;
	(void)context->index;

	for (;;) {
		se_worker_task_claim claim = {0};

		s_mutex_lock(&pool->mutex);
		while (pool->queue_count == 0u && pool->running) {
			s_cond_wait(&pool->work_ready, &pool->mutex);
		}
		if (pool->queue_count == 0u && !pool->running) {
			s_mutex_unlock(&pool->mutex);
			break;
		}
		if (!se_worker_try_claim_task_locked(pool, &claim)) {
			s_mutex_unlock(&pool->mutex);
			continue;
		}
		s_mutex_unlock(&pool->mutex);

		se_worker_execute_claim(pool, &claim);
	}

	return NULL;
}

static void* se_worker_parallel_for_task(void* user_data) {
	se_worker_parallel_for_state* state = (se_worker_parallel_for_state*)user_data;
	if (!state || !state->fn) {
		return NULL;
	}

	for (;;) {
		u32 start = 0u;
		u32 end = 0u;

		s_mutex_lock(&state->mutex);
		if (state->next_index >= state->count) {
			s_mutex_unlock(&state->mutex);
			break;
		}

		start = state->next_index;
		const u32 remaining = state->count - start;
		const u32 advance = state->batch_size < remaining ? state->batch_size : remaining;
		end = start + advance;
		state->next_index = end;
		s_mutex_unlock(&state->mutex);

		for (u32 index = start; index < end; ++index) {
			state->fn(index, state->user_data);
		}
	}

	return NULL;
}

static b8 se_worker_wait_for_queue_room(se_worker_pool* pool) {
	if (!pool) {
		return false;
	}

	s_mutex_lock(&pool->mutex);
	while (pool->running && !pool->stopping && pool->queue_count >= pool->config.queue_capacity) {
		s_cond_wait(&pool->state_changed, &pool->mutex);
	}
	const b8 ok = pool->running && !pool->stopping && pool->queue_count < pool->config.queue_capacity;
	s_mutex_unlock(&pool->mutex);
	return ok;
}

static void se_worker_shutdown_threads(se_worker_pool* pool) {
	if (!pool || !pool->mutex_ready) {
		return;
	}

	s_mutex_lock(&pool->mutex);
	pool->stopping = true;
	pool->running = false;
	if (pool->work_ready_ready) {
		s_cond_broadcast(&pool->work_ready);
	}
	if (pool->state_changed_ready) {
		s_cond_broadcast(&pool->state_changed);
	}
	s_mutex_unlock(&pool->mutex);

	for (u32 i = 0u; i < pool->started_threads; ++i) {
		(void)s_thread_join(&pool->threads[i], NULL);
	}
	pool->started_threads = 0u;
}

static void se_worker_cleanup_pool(se_worker_pool* pool) {
	if (!pool) {
		return;
	}

	se_worker_shutdown_threads(pool);
	if (pool->state_changed_ready) {
		s_cond_destroy(&pool->state_changed);
		pool->state_changed_ready = false;
	}
	if (pool->work_ready_ready) {
		s_cond_destroy(&pool->work_ready);
		pool->work_ready_ready = false;
	}
	if (pool->mutex_ready) {
		s_mutex_destroy(&pool->mutex);
		pool->mutex_ready = false;
	}
	s_array_clear(&pool->tasks);
	free(pool->queue);
	free(pool->thread_contexts);
	free(pool->threads);
	free(pool);
}

se_worker_pool* se_worker_create(const se_worker_config* config) {
	const se_worker_config resolved = se_worker_resolve_config(config);

	se_worker_pool* pool = malloc(sizeof(se_worker_pool));
	if (!pool) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(pool, 0, sizeof(*pool));
	pool->config = resolved;
	s_array_init(&pool->tasks);
	s_array_reserve(&pool->tasks, pool->config.max_tasks);

	pool->threads = calloc(pool->config.thread_count, sizeof(*pool->threads));
	pool->thread_contexts = calloc(pool->config.thread_count, sizeof(*pool->thread_contexts));
	pool->queue = calloc(pool->config.queue_capacity, sizeof(*pool->queue));
	if (!pool->threads || !pool->thread_contexts || !pool->queue) {
		se_worker_cleanup_pool(pool);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	if (!s_mutex_init(&pool->mutex)) {
		se_worker_cleanup_pool(pool);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}
	pool->mutex_ready = true;

	if (!s_cond_init(&pool->work_ready)) {
		se_worker_cleanup_pool(pool);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}
	pool->work_ready_ready = true;

	if (!s_cond_init(&pool->state_changed)) {
		se_worker_cleanup_pool(pool);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}
	pool->state_changed_ready = true;
	pool->running = true;

	for (u32 i = 0u; i < pool->config.thread_count; ++i) {
		pool->thread_contexts[i].pool = pool;
		pool->thread_contexts[i].index = i;
		if (!s_thread_create(&pool->threads[i], se_worker_thread_main, &pool->thread_contexts[i])) {
			se_worker_cleanup_pool(pool);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			return NULL;
		}
		pool->started_threads++;
	}

	se_set_last_error(SE_RESULT_OK);
	return pool;
}

void se_worker_destroy(se_worker_pool* pool) {
	if (!pool) {
		return;
	}
	if (g_se_worker_tls_pool == pool) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return;
	}

	s_mutex_lock(&pool->mutex);
	pool->stopping = true;
	s_cond_broadcast(&pool->work_ready);
	s_cond_broadcast(&pool->state_changed);
	s_mutex_unlock(&pool->mutex);

	(void)se_worker_wait_idle(pool);

	se_worker_cleanup_pool(pool);
}

se_worker_task se_worker_submit(se_worker_pool* pool, se_worker_task_fn fn, void* user_data) {
	if (!pool || !fn) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return SE_WORKER_TASK_NULL;
	}

	se_worker_task task = SE_WORKER_TASK_NULL;
	s_mutex_lock(&pool->mutex);
	const se_worker_submit_result result = se_worker_submit_locked(pool, fn, user_data, &task);
	s_mutex_unlock(&pool->mutex);

	if (result != SE_WORKER_SUBMIT_OK) {
		se_set_last_error(se_worker_submit_result_to_error(result));
		return SE_WORKER_TASK_NULL;
	}

	se_set_last_error(SE_RESULT_OK);
	return task;
}

b8 se_worker_poll(se_worker_pool* pool, se_worker_task task, b8* out_done, void** out_result) {
	if (out_done) {
		*out_done = false;
	}
	if (out_result) {
		*out_result = NULL;
	}
	if (!pool || task == SE_WORKER_TASK_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	se_worker_task_record* record = s_array_get(&pool->tasks, task);
	if (!record) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	const b8 done = record->state == SE_WORKER_TASK_STATE_DONE;
	if (out_done) {
		*out_done = done;
	}
	if (done && out_result) {
		*out_result = record->result;
	}
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_worker_wait(se_worker_pool* pool, se_worker_task task, void** out_result) {
	if (out_result) {
		*out_result = NULL;
	}
	if (!pool || task == SE_WORKER_TASK_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (g_se_worker_tls_pool == pool && g_se_worker_tls_task == task) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}

	for (;;) {
		se_worker_task_claim claim = {0};

		s_mutex_lock(&pool->mutex);
		se_worker_task_record* record = s_array_get(&pool->tasks, task);
		if (!record) {
			s_mutex_unlock(&pool->mutex);
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return false;
		}
		if (record->state == SE_WORKER_TASK_STATE_DONE) {
			if (out_result) {
				*out_result = record->result;
			}
			s_mutex_unlock(&pool->mutex);
			se_set_last_error(SE_RESULT_OK);
			return true;
		}
		if (g_se_worker_tls_pool == pool && se_worker_try_claim_task_locked(pool, &claim)) {
			s_mutex_unlock(&pool->mutex);
			se_worker_execute_claim(pool, &claim);
			continue;
		}
		s_cond_wait(&pool->state_changed, &pool->mutex);
		s_mutex_unlock(&pool->mutex);
	}
}

b8 se_worker_release(se_worker_pool* pool, se_worker_task task) {
	if (!pool || task == SE_WORKER_TASK_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	se_worker_task_record* record = s_array_get(&pool->tasks, task);
	if (!record) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (record->state != SE_WORKER_TASK_STATE_DONE) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	(void)s_array_remove(&pool->tasks, task);
	s_cond_broadcast(&pool->state_changed);
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_worker_wait_idle(se_worker_pool* pool) {
	if (!pool) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (g_se_worker_tls_pool == pool) {
		se_set_last_error(SE_RESULT_UNSUPPORTED);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	while (pool->queue_count > 0u || pool->active_tasks > 0u) {
		s_cond_wait(&pool->state_changed, &pool->mutex);
	}
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_worker_parallel_for(se_worker_pool* pool, u32 count, u32 batch_size, se_worker_for_fn fn, void* user_data) {
	if (!pool || !fn) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	if (count == 0u) {
		se_set_last_error(SE_RESULT_OK);
		return true;
	}

	se_worker_parallel_for_state state = {0};
	state.fn = fn;
	state.user_data = user_data;
	state.count = count;
	state.batch_size = batch_size == 0u ? 1u : batch_size;

	if (!s_mutex_init(&state.mutex)) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return false;
	}

	const u32 batch_count = 1u + ((count - 1u) / state.batch_size);
	u32 target_jobs = pool->config.thread_count;
	if (target_jobs > batch_count) {
		target_jobs = batch_count;
	}
	if (target_jobs > pool->config.max_tasks) {
		target_jobs = pool->config.max_tasks;
	}
	if (target_jobs == 0u) {
		s_mutex_destroy(&state.mutex);
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return false;
	}

	se_worker_task* tasks = calloc(target_jobs, sizeof(*tasks));
	if (!tasks) {
		s_mutex_destroy(&state.mutex);
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return false;
	}

	u32 submitted = 0u;
	b8 ok = true;
	se_result failure = SE_RESULT_OK;

	while (submitted < target_jobs) {
		s_mutex_lock(&pool->mutex);
		const se_worker_submit_result submit_result =
			se_worker_submit_locked(pool, se_worker_parallel_for_task, &state, &tasks[submitted]);
		s_mutex_unlock(&pool->mutex);

		if (submit_result == SE_WORKER_SUBMIT_OK) {
			submitted++;
			continue;
		}
		if (submit_result == SE_WORKER_SUBMIT_QUEUE_FULL) {
			if (!se_worker_wait_for_queue_room(pool)) {
				ok = false;
				failure = SE_RESULT_UNSUPPORTED;
				break;
			}
			continue;
		}
		if (submit_result == SE_WORKER_SUBMIT_TASKS_FULL && submitted > 0u) {
			break;
		}
		ok = false;
		failure = se_worker_submit_result_to_error(submit_result);
		break;
	}

	for (u32 i = 0u; i < submitted; ++i) {
		if (!se_worker_wait(pool, tasks[i], NULL) && ok) {
			ok = false;
			failure = se_get_last_error();
		}
	}
	for (u32 i = 0u; i < submitted; ++i) {
		if (tasks[i] != SE_WORKER_TASK_NULL && !se_worker_release(pool, tasks[i]) && ok) {
			ok = false;
			failure = se_get_last_error();
		}
	}

	free(tasks);
	s_mutex_destroy(&state.mutex);

	se_set_last_error(ok ? SE_RESULT_OK : failure);
	return ok;
}

b8 se_worker_get_diagnostics(se_worker_pool* pool, se_worker_diagnostics* out_diagnostics) {
	if (!pool || !out_diagnostics) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	memset(out_diagnostics, 0, sizeof(*out_diagnostics));
	out_diagnostics->thread_count = pool->config.thread_count;
	out_diagnostics->queue_capacity = pool->config.queue_capacity;
	out_diagnostics->max_tasks = pool->config.max_tasks;
	out_diagnostics->pending_tasks = pool->queue_count;
	out_diagnostics->active_tasks = pool->active_tasks;
	out_diagnostics->tracked_tasks = (u32)s_array_get_size(&pool->tasks);
	out_diagnostics->submitted_tasks = pool->submitted_tasks;
	out_diagnostics->completed_tasks = pool->completed_tasks;
	out_diagnostics->running = pool->running;
	out_diagnostics->stopping = pool->stopping;
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}
