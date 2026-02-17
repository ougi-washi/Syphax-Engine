---
title: se_window
summary: Window lifecycle, per-frame APIs, and render-thread-aware present behavior.
prerequisites:
  - Public header include path available.
---

# se_window


## When to use this

Use `se_window` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_window.h"
se_window_begin_frame(window);
se_window_end_frame(window);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call `se_window_begin_frame` / `se_window_end_frame` each frame; this stays valid for both single-thread and render-thread-backed paths.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_window.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.
- Calling `se_window_present` or `se_window_set_current_context` while dedicated render-thread mode is active.

## Related pages

- [Deep dive path page](../path/window.md)
- [API module page](../api-reference/modules/se_window.md)
- [API: se_render_frame.h](../api-reference/modules/se_render_frame.md)
- [API: se_render_thread.h](../api-reference/modules/se_render_thread.md)
- [Example: hello_text](../examples/default/hello_text.md)
- [Example: input_actions](../examples/default/input_actions.md)
