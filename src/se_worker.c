// Syphax-Engine - Ougi Washi

#include "se_worker.h"

#include "syphax/s_array.h"
#include "syphax/s_thread.h"

#include <stdlib.h>

typedef enum {
	SE_WORKER_TASK_STATE_PENDING = 0,
	SE_WORKER_TASK_STATE_RUNNING,
	SE_WORKER_TASK_STATE_DONE
} se_worker_task_state;

typedef struct {
	se_worker_task_fn fn;
	void* user_data;
	void* result;
	se_worker_task_state state;
} se_worker_task_entry;

typedef s_array(se_worker_task_entry, se_worker_task_entries);
typedef s_array(se_worker_task, se_worker_task_queue);
typedef s_array(s_thread, se_worker_threads);

typedef struct {
	se_worker_for_fn fn;
	void* user_data;
	u32 begin;
	u32 end;
} se_worker_for_chunk;

struct se_worker_pool {
	s_mutex mutex;
	s_cond work_available;
	s_cond task_done;
	se_worker_task_entries tasks;
	se_worker_task_queue pending;
	se_worker_threads threads;
	u32 thread_count;
	u32 queue_capacity;
	u32 max_tasks;
	u32 active_tasks;
	u64 submitted_tasks;
	u64 completed_tasks;
	b8 mutex_ready : 1;
	b8 work_available_ready : 1;
	b8 task_done_ready : 1;
	b8 running : 1;
	b8 stopping : 1;
};

static b8 se_worker_has_work_locked(const se_worker_pool* pool) {
	return pool && s_array_get_size(&pool->pending) > 0;
}

static se_worker_task se_worker_pop_pending_locked(se_worker_pool* pool) {
	if (!pool) {
		return SE_WORKER_TASK_NULL;
	}
	const sz count = s_array_get_size(&pool->pending);
	if (count == 0) {
		return SE_WORKER_TASK_NULL;
	}
	const s_handle pending_handle = s_array_handle(&pool->pending, (u32)(count - 1));
	const se_worker_task* task_handle_ptr = s_array_get(&pool->pending, pending_handle);
	if (!task_handle_ptr) {
		return SE_WORKER_TASK_NULL;
	}
	const se_worker_task task_handle = *task_handle_ptr;
	s_array_remove(&pool->pending, pending_handle);
	return task_handle;
}

static void* se_worker_thread_main(void* arg) {
	se_worker_pool* pool = (se_worker_pool*)arg;
	if (!pool) {
		return NULL;
	}

	for (;;) {
		s_mutex_lock(&pool->mutex);
		while (!pool->stopping && !se_worker_has_work_locked(pool)) {
			s_cond_wait(&pool->work_available, &pool->mutex);
		}
		if (pool->stopping && !se_worker_has_work_locked(pool)) {
			s_mutex_unlock(&pool->mutex);
			break;
		}

		const se_worker_task task_handle = se_worker_pop_pending_locked(pool);
		se_worker_task_entry* task = s_array_get(&pool->tasks, task_handle);
		if (!task) {
			s_cond_broadcast(&pool->task_done);
			s_mutex_unlock(&pool->mutex);
			continue;
		}
		task->state = SE_WORKER_TASK_STATE_RUNNING;
		pool->active_tasks++;
		s_mutex_unlock(&pool->mutex);

		void* result = task->fn(task->user_data);

		s_mutex_lock(&pool->mutex);
		task = s_array_get(&pool->tasks, task_handle);
		if (task) {
			task->result = result;
			task->state = SE_WORKER_TASK_STATE_DONE;
		}
		if (pool->active_tasks > 0) {
			pool->active_tasks--;
		}
		pool->completed_tasks++;
		s_cond_broadcast(&pool->task_done);
		s_mutex_unlock(&pool->mutex);
	}

	return NULL;
}

static void se_worker_join_threads(se_worker_pool* pool) {
	if (!pool) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(&pool->threads); ++i) {
		const s_handle thread_handle = s_array_handle(&pool->threads, (u32)i);
		s_thread* thread = s_array_get(&pool->threads, thread_handle);
		if (!thread) {
			continue;
		}
		(void)s_thread_join(thread, NULL);
	}
	s_array_clear(&pool->threads);
}

static void se_worker_cleanup(se_worker_pool* pool) {
	if (!pool) {
		return;
	}
	s_array_clear(&pool->tasks);
	s_array_clear(&pool->pending);
	s_array_clear(&pool->threads);
	if (pool->task_done_ready) {
		s_cond_destroy(&pool->task_done);
		pool->task_done_ready = false;
	}
	if (pool->work_available_ready) {
		s_cond_destroy(&pool->work_available);
		pool->work_available_ready = false;
	}
	if (pool->mutex_ready) {
		s_mutex_destroy(&pool->mutex);
		pool->mutex_ready = false;
	}
	free(pool);
}

static void se_worker_apply_defaults(se_worker_config* out_config, const se_worker_config* config) {
	if (!out_config) {
		return;
	}
	*out_config = config ? *config : SE_WORKER_CONFIG_DEFAULTS;
	if (out_config->thread_count == 0u) {
		out_config->thread_count = 1u;
	}
	if (out_config->queue_capacity == 0u) {
		out_config->queue_capacity = 1u;
	}
	if (out_config->max_tasks == 0u) {
		out_config->max_tasks = out_config->queue_capacity;
	}
	if (out_config->max_tasks < out_config->queue_capacity) {
		out_config->max_tasks = out_config->queue_capacity;
	}
}

se_worker_pool* se_worker_create(const se_worker_config* config) {
	se_worker_config cfg = {0};
	se_worker_apply_defaults(&cfg, config);

	se_worker_pool* pool = calloc(1, sizeof(*pool));
	if (!pool) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}

	s_array_init(&pool->tasks);
	s_array_init(&pool->pending);
	s_array_init(&pool->threads);

	if (!s_mutex_init(&pool->mutex)) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		se_worker_cleanup(pool);
		return NULL;
	}
	pool->mutex_ready = true;
	if (!s_cond_init(&pool->work_available)) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		se_worker_cleanup(pool);
		return NULL;
	}
	pool->work_available_ready = true;
	if (!s_cond_init(&pool->task_done)) {
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		se_worker_cleanup(pool);
		return NULL;
	}
	pool->task_done_ready = true;

	pool->thread_count = cfg.thread_count;
	pool->queue_capacity = cfg.queue_capacity;
	pool->max_tasks = cfg.max_tasks;
	pool->running = true;
	pool->stopping = false;
	pool->active_tasks = 0u;
	pool->submitted_tasks = 0u;
	pool->completed_tasks = 0u;

	s_array_reserve(&pool->pending, cfg.queue_capacity);
	s_array_reserve(&pool->tasks, cfg.max_tasks);
	s_array_reserve(&pool->threads, cfg.thread_count);

	for (u32 i = 0; i < cfg.thread_count; ++i) {
		const s_handle thread_slot = s_array_increment(&pool->threads);
		s_thread* thread = s_array_get(&pool->threads, thread_slot);
		if (!thread || !s_thread_create(thread, se_worker_thread_main, pool)) {
			s_mutex_lock(&pool->mutex);
			pool->stopping = true;
			s_cond_broadcast(&pool->work_available);
			s_mutex_unlock(&pool->mutex);
			se_worker_join_threads(pool);
			se_set_last_error(SE_RESULT_BACKEND_FAILURE);
			se_worker_cleanup(pool);
			return NULL;
		}
	}

	se_set_last_error(SE_RESULT_OK);
	return pool;
}

void se_worker_destroy(se_worker_pool* pool) {
	if (!pool) {
		return;
	}

	s_mutex_lock(&pool->mutex);
	pool->stopping = true;
	pool->running = false;
	s_cond_broadcast(&pool->work_available);
	s_cond_broadcast(&pool->task_done);
	s_mutex_unlock(&pool->mutex);

	se_worker_join_threads(pool);
	se_worker_cleanup(pool);
}

se_worker_task se_worker_submit(se_worker_pool* pool, se_worker_task_fn fn, void* user_data) {
	if (!pool || !fn) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return SE_WORKER_TASK_NULL;
	}

	s_mutex_lock(&pool->mutex);
	if (!pool->running || pool->stopping) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return SE_WORKER_TASK_NULL;
	}
	if (s_array_get_size(&pool->pending) >= pool->queue_capacity || s_array_get_size(&pool->tasks) >= pool->max_tasks) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return SE_WORKER_TASK_NULL;
	}

	se_worker_task_entry task = {0};
	task.fn = fn;
	task.user_data = user_data;
	task.result = NULL;
	task.state = SE_WORKER_TASK_STATE_PENDING;
	const se_worker_task task_handle = s_array_add(&pool->tasks, task);
	s_array_add(&pool->pending, task_handle);
	pool->submitted_tasks++;
	s_cond_signal(&pool->work_available);
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return task_handle;
}

b8 se_worker_poll(se_worker_pool* pool, se_worker_task task, b8* out_done, void** out_result) {
	if (!pool || task == SE_WORKER_TASK_NULL || !out_done) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	se_worker_task_entry* entry = s_array_get(&pool->tasks, task);
	if (!entry) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	*out_done = entry->state == SE_WORKER_TASK_STATE_DONE;
	if (out_result) {
		*out_result = *out_done ? entry->result : NULL;
	}
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_worker_wait(se_worker_pool* pool, se_worker_task task, void** out_result) {
	if (!pool || task == SE_WORKER_TASK_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	se_worker_task_entry* entry = s_array_get(&pool->tasks, task);
	if (!entry) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}

	while (entry->state != SE_WORKER_TASK_STATE_DONE) {
		s_cond_wait(&pool->task_done, &pool->mutex);
		entry = s_array_get(&pool->tasks, task);
		if (!entry) {
			s_mutex_unlock(&pool->mutex);
			se_set_last_error(SE_RESULT_NOT_FOUND);
			return false;
		}
	}

	if (out_result) {
		*out_result = entry->result;
	}
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_worker_release(se_worker_pool* pool, se_worker_task task) {
	if (!pool || task == SE_WORKER_TASK_NULL) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	se_worker_task_entry* entry = s_array_get(&pool->tasks, task);
	if (!entry) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_NOT_FOUND);
		return false;
	}
	if (entry->state != SE_WORKER_TASK_STATE_DONE) {
		s_mutex_unlock(&pool->mutex);
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}
	s_array_remove(&pool->tasks, task);
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_worker_wait_idle(se_worker_pool* pool) {
	if (!pool) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	while (s_array_get_size(&pool->pending) > 0 || pool->active_tasks > 0u) {
		s_cond_wait(&pool->task_done, &pool->mutex);
	}
	s_mutex_unlock(&pool->mutex);

	se_set_last_error(SE_RESULT_OK);
	return true;
}

static void* se_worker_parallel_for_task(void* user_data) {
	se_worker_for_chunk* chunk = (se_worker_for_chunk*)user_data;
	if (!chunk || !chunk->fn) {
		free(chunk);
		return NULL;
	}

	for (u32 i = chunk->begin; i < chunk->end; ++i) {
		chunk->fn(i, chunk->user_data);
	}
	free(chunk);
	return NULL;
}

typedef s_array(se_worker_task, se_worker_task_list);

static void se_worker_collect_tasks(se_worker_pool* pool, se_worker_task_list* tasks) {
	if (!pool || !tasks) {
		return;
	}
	for (sz i = 0; i < s_array_get_size(tasks); ++i) {
		const s_handle list_handle = s_array_handle(tasks, (u32)i);
		const se_worker_task* task_ptr = s_array_get(tasks, list_handle);
		if (!task_ptr) {
			continue;
		}
		(void)se_worker_wait(pool, *task_ptr, NULL);
		(void)se_worker_release(pool, *task_ptr);
	}
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
	if (batch_size == 0u) {
		batch_size = 1u;
	}

	if (pool->thread_count <= 1u || count <= batch_size) {
		for (u32 i = 0; i < count; ++i) {
			fn(i, user_data);
		}
		se_set_last_error(SE_RESULT_OK);
		return true;
	}

	se_worker_task_list tasks = {0};
	s_array_init(&tasks);

	for (u32 begin = 0u; begin < count; begin += batch_size) {
		se_worker_for_chunk* chunk = malloc(sizeof(*chunk));
		if (!chunk) {
			se_worker_collect_tasks(pool, &tasks);
			s_array_clear(&tasks);
			se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
			return false;
		}
		chunk->fn = fn;
		chunk->user_data = user_data;
		chunk->begin = begin;
		chunk->end = begin + batch_size;
		if (chunk->end > count) {
			chunk->end = count;
		}

		se_worker_task task = se_worker_submit(pool, se_worker_parallel_for_task, chunk);
		if (task == SE_WORKER_TASK_NULL) {
			se_result error = se_get_last_error();
			free(chunk);
			se_worker_collect_tasks(pool, &tasks);
			s_array_clear(&tasks);
			se_set_last_error(error);
			return false;
		}
		s_array_add(&tasks, task);
	}

	b8 ok = true;
	for (sz i = 0; i < s_array_get_size(&tasks); ++i) {
		const s_handle list_handle = s_array_handle(&tasks, (u32)i);
		const se_worker_task* task_ptr = s_array_get(&tasks, list_handle);
		if (!task_ptr) {
			ok = false;
			continue;
		}
		if (!se_worker_wait(pool, *task_ptr, NULL)) {
			ok = false;
		}
	}
	for (sz i = 0; i < s_array_get_size(&tasks); ++i) {
		const s_handle list_handle = s_array_handle(&tasks, (u32)i);
		const se_worker_task* task_ptr = s_array_get(&tasks, list_handle);
		if (!task_ptr) {
			ok = false;
			continue;
		}
		if (!se_worker_release(pool, *task_ptr)) {
			ok = false;
		}
	}
	s_array_clear(&tasks);

	if (!ok) {
		return false;
	}
	se_set_last_error(SE_RESULT_OK);
	return true;
}

b8 se_worker_get_diagnostics(se_worker_pool* pool, se_worker_diagnostics* out_diagnostics) {
	if (!pool || !out_diagnostics) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return false;
	}

	s_mutex_lock(&pool->mutex);
	out_diagnostics->thread_count = pool->thread_count;
	out_diagnostics->queue_capacity = pool->queue_capacity;
	out_diagnostics->max_tasks = pool->max_tasks;
	out_diagnostics->pending_tasks = (u32)s_array_get_size(&pool->pending);
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
