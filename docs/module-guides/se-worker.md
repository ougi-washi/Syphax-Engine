---
title: se_worker
summary: Background task pool, tracked task handles, and parallel-for helpers.
prerequisites:
  - Public header include path available.
---

# se_worker

## When to use this

Use `se_worker` for CPU-side background jobs or batched data processing that should not run on the render or window thread.

## Minimal working snippet

```c
#include "se_worker.h"

se_worker_pool* pool = se_worker_create(NULL);
se_worker_parallel_for(pool, count, 64u, fn, user_data);
```

## Step-by-step explanation

1. Create one pool with a config that matches the workload instead of spawning ad-hoc threads per feature.
1. Use `submit` / `poll` / `wait` / `release` when each task has a tracked result, or use `parallel_for` for uniform batched work.
1. Wait for idle work before shutdown and destroy the pool only after no caller still depends on its task handles.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_worker.md).
1. Continue with [Simulation and advanced](../simulation-and-advanced/index.md).

</div>

## Common mistakes

- Forgetting to release a completed task handle after consuming its result.
- Waiting on or polling task handles after the pool has been destroyed.
- Using worker callbacks to touch APIs that must remain on the main, window, or render thread.

## Related pages

- [Simulation and advanced overview](../simulation-and-advanced/index.md)
- [Module guide: se_simulation](se-simulation.md)
- [Module guide: se_render_thread](se-render-thread.md)
- [API: se_defines.h](../api-reference/modules/se_defines.md)
