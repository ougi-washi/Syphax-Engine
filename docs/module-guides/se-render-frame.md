---
title: se_render_frame
summary: Manual frame submission helpers and queue-facing frame statistics.
prerequisites:
  - Public header include path available.
---

# se_render_frame

## When to use this

Use `se_render_frame` when you need direct control over frame submission or want queue-facing frame stats instead of relying only on the higher-level window loop.

## Minimal working snippet

```c
#include "se_render_frame.h"

se_render_frame_begin(window);
se_render_frame_submit(window);
```

## Step-by-step explanation

1. Reach for this module only when you are intentionally managing frame submission yourself or collecting queue stats explicitly.
1. Use `se_render_frame_wait_presented(...)` and `se_render_frame_get_stats(...)` for diagnostics or synchronization around the present path.
1. Prefer the higher-level `se_window_begin_frame(...)` / `se_window_end_frame(...)` loop when you do not need manual control.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_render_frame.md).
1. Compare with the [Window path page](../path/window.md).

</div>

## Common mistakes

- Mixing `se_render_frame_submit(...)` with `se_window_end_frame(...)` in the same frame.
- Expecting useful stats before a queue-backed path has actually presented frames.
- Treating this API as a replacement for window creation or render-thread lifecycle management.

## Related pages

- [Path: Window](../path/window.md)
- [Module guide: se_window](se-window.md)
- [Module guide: se_render_thread](se-render-thread.md)
- [Debug and performance overview](../debug-and-performance/index.md)
