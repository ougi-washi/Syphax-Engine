---
title: se_debug
summary: Logging, tracing, overlay, and last-frame timing diagnostics.
prerequisites:
  - Public header include path available.
---

# se_debug

## When to use this

Use `se_debug` to control log volume, collect trace spans, inspect last-frame timings, and render the built-in debug overlay.

## Minimal working snippet

```c
#include "se_debug.h"

se_debug_set_level(SE_DEBUG_LEVEL_INFO);
se_debug_dump_frame_timing_lines(buffer, sizeof(buffer));
```

## Step-by-step explanation

1. Set the log level and category mask early so runtime output matches the problem you are chasing.
1. Wrap meaningful work with trace spans or query the last-frame timing snapshot after running a frame.
1. Use dump helpers or the overlay during validation, then turn noisy tracing back down before shipping or profiling hot loops.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_debug.md).
1. Continue with the [Debug path page](../path/debug.md).

</div>

## Common mistakes

- Reading timing output before any frame has completed.
- Leaving detailed trace logging enabled in hot loops during performance checks.
- Assuming `se_debug` replaces the need for backend or render-queue diagnostics when the bottleneck is submit/present behavior.

## Related pages

- [Path: Debug](../path/debug.md)
- [Debug and performance overview](../debug-and-performance/index.md)
- [Module guide: se_render_frame](se-render-frame.md)
- [Module guide: se_render_thread](se-render-thread.md)
- [Example: debug_tools](../examples/advanced/debug_tools.md)
