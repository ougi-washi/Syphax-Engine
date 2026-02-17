<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_worker.h

Source header: `include/se_worker.h`

## Overview

Public declarations exposed by this header.

This page is generated from `include/se_worker.h` and is deterministic.

## Functions

### `se_worker_create`

<div class="api-signature">

```c
extern se_worker_pool* se_worker_create(const se_worker_config* config);
```

</div>

No inline description found in header comments.

### `se_worker_destroy`

<div class="api-signature">

```c
extern void se_worker_destroy(se_worker_pool* pool);
```

</div>

No inline description found in header comments.

### `se_worker_get_diagnostics`

<div class="api-signature">

```c
extern b8 se_worker_get_diagnostics(se_worker_pool* pool, se_worker_diagnostics* out_diagnostics);
```

</div>

No inline description found in header comments.

### `se_worker_parallel_for`

<div class="api-signature">

```c
extern b8 se_worker_parallel_for(se_worker_pool* pool, u32 count, u32 batch_size, se_worker_for_fn fn, void* user_data);
```

</div>

No inline description found in header comments.

### `se_worker_poll`

<div class="api-signature">

```c
extern b8 se_worker_poll(se_worker_pool* pool, se_worker_task task, b8* out_done, void** out_result);
```

</div>

No inline description found in header comments.

### `se_worker_release`

<div class="api-signature">

```c
extern b8 se_worker_release(se_worker_pool* pool, se_worker_task task);
```

</div>

No inline description found in header comments.

### `se_worker_submit`

<div class="api-signature">

```c
extern se_worker_task se_worker_submit(se_worker_pool* pool, se_worker_task_fn fn, void* user_data);
```

</div>

No inline description found in header comments.

### `se_worker_wait`

<div class="api-signature">

```c
extern b8 se_worker_wait(se_worker_pool* pool, se_worker_task task, void** out_result);
```

</div>

No inline description found in header comments.

### `se_worker_wait_idle`

<div class="api-signature">

```c
extern b8 se_worker_wait_idle(se_worker_pool* pool);
```

</div>

No inline description found in header comments.

## Enums

No enums found in this header.

## Typedefs

### `se_worker_config`

<div class="api-signature">

```c
typedef struct { u32 thread_count; u32 queue_capacity; u32 max_tasks; } se_worker_config;
```

</div>

No inline description found in header comments.

### `se_worker_diagnostics`

<div class="api-signature">

```c
typedef struct { u32 thread_count; u32 queue_capacity; u32 max_tasks; u32 pending_tasks; u32 active_tasks; u32 tracked_tasks; u64 submitted_tasks; u64 completed_tasks; b8 running : 1; b8 stopping : 1; } se_worker_diagnostics;
```

</div>

No inline description found in header comments.

### `se_worker_for_fn`

<div class="api-signature">

```c
typedef void (*se_worker_for_fn)(u32 index, void* user_data);
```

</div>

No inline description found in header comments.

### `se_worker_pool`

<div class="api-signature">

```c
typedef struct se_worker_pool se_worker_pool;
```

</div>

No inline description found in header comments.

### `se_worker_task`

<div class="api-signature">

```c
typedef s_handle se_worker_task;
```

</div>

No inline description found in header comments.

### `se_worker_task_fn`

<div class="api-signature">

```c
typedef void* (*se_worker_task_fn)(void* user_data);
```

</div>

No inline description found in header comments.
