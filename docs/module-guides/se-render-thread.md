---
title: se_render_thread
summary: Dedicated render-thread lifecycle and queue diagnostics for a window.
prerequisites:
  - Public header include path available.
---

# se_render_thread

## When to use this

Use `se_render_thread` when a window should hand off render submission and present work to the dedicated queue thread.

## Minimal working snippet

```c
#include "se_render_thread.h"

se_render_thread_handle thread = se_render_thread_start(window, NULL);
se_render_thread_wait_idle(thread);
```

## Step-by-step explanation

1. Start the render thread only after the target window exists and you have decided that queue-backed rendering is the path you want to validate.
1. Keep using the canonical window loop while the render thread handles the submit/present side underneath.
1. Query diagnostics when investigating stalls or queue depth, then stop the thread explicitly during teardown or controlled shutdown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_render_thread.md).
1. Compare with the [Window path page](../path/window.md).

</div>

## Common mistakes

- Calling direct present or context-switch APIs while the dedicated render thread is active.
- Forgetting to stop the thread during explicit teardown flows.
- Treating the returned handle as a general multi-window thread pool instead of the queue controller for one window path.

## Related pages

- [Path: Window](../path/window.md)
- [Module guide: se_window](se-window.md)
- [Module guide: se_render_frame](se-render-frame.md)
- [Debug and performance overview](../debug-and-performance/index.md)
