---
title: se_window
summary: Window creation, canonical frame loop, and per-window diagnostics.
prerequisites:
  - Public header include path available.
---

# se_window

## When to use this

Use `se_window` to create the app window, drive the canonical frame loop, and query per-window input and timing state.

## Minimal working snippet

```c
#include "se_window.h"

se_window_handle window = se_window_create("App", 1280, 720);
se_window_begin_frame(window);
se_window_end_frame(window);
```

## Step-by-step explanation

1. Create a context first, then create the window and validate the returned handle immediately.
1. Prefer `se_window_begin_frame` / `se_window_end_frame` in new code instead of mixing legacy tick/present calls.
1. Use the window as the source of size, aspect, cursor, text input, and diagnostics, then tear down through `se_context_destroy(...)` for the final owner context.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_window.md).
1. Continue with the [Window path page](../path/window.md).

</div>

## Common mistakes

- Creating a window before creating a context, or creating windows under a second active owning context.
- Mixing `se_window_end_frame(window)` with manual `se_window_present(...)` or `se_render_frame_submit(...)` in the same frame.
- Destroying the final window manually while the context still owns other live resources.

## Related pages

- [Start Here: First window](../start-here/first-window.md)
- [Path: Window](../path/window.md)
- [Module guide: se_render_frame](se-render-frame.md)
- [Module guide: se_render_thread](se-render-thread.md)
- [Example: hello_text](../examples/default/hello_text.md)
- [Example: input_actions](../examples/default/input_actions.md)
