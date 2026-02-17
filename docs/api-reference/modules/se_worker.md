<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->
# se_worker.h

Source header: `include/se_worker.h`

## Overview

Public declarations exposed by this header.

This page is generated from `include/se_worker.h` and is deterministic.

## Functions

No functions found in this header.

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
