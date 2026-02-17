// Syphax-Engine - Ougi Washi

#ifndef SE_WORKER_H
#define SE_WORKER_H

#include "se.h"
#include "se_defines.h"

#define SE_WORKER_TASK_NULL S_HANDLE_NULL

typedef s_handle se_worker_task;

typedef struct se_worker_pool se_worker_pool;

typedef void* (*se_worker_task_fn)(void* user_data);
typedef void (*se_worker_for_fn)(u32 index, void* user_data);

typedef struct {
	u32 thread_count;
	u32 queue_capacity;
	u32 max_tasks;
} se_worker_config;

#define SE_WORKER_CONFIG_DEFAULTS ((se_worker_config){ \
	.thread_count = 4u, \
	.queue_capacity = 1024u, \
	.max_tasks = 4096u \
})

typedef struct {
	u32 thread_count;
	u32 queue_capacity;
	u32 max_tasks;
	u32 pending_tasks;
	u32 active_tasks;
	u32 tracked_tasks;
	u64 submitted_tasks;
	u64 completed_tasks;
	b8 running : 1;
	b8 stopping : 1;
} se_worker_diagnostics;

extern se_worker_pool* se_worker_create(const se_worker_config* config);
extern void se_worker_destroy(se_worker_pool* pool);

extern se_worker_task se_worker_submit(se_worker_pool* pool, se_worker_task_fn fn, void* user_data);
extern b8 se_worker_poll(se_worker_pool* pool, se_worker_task task, b8* out_done, void** out_result);
extern b8 se_worker_wait(se_worker_pool* pool, se_worker_task task, void** out_result);
extern b8 se_worker_release(se_worker_pool* pool, se_worker_task task);
extern b8 se_worker_wait_idle(se_worker_pool* pool);

extern b8 se_worker_parallel_for(se_worker_pool* pool, u32 count, u32 batch_size, se_worker_for_fn fn, void* user_data);

extern b8 se_worker_get_diagnostics(se_worker_pool* pool, se_worker_diagnostics* out_diagnostics);

#endif // SE_WORKER_H
