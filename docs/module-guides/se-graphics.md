---
title: se_graphics
summary: Low-level clear and render-state helpers used beneath higher-level modules.
prerequisites:
  - Public header include path available.
---

# se_graphics

## When to use this

Use `se_graphics` for simple render-state helpers such as clearing the current target, especially when you are composing your own scene, UI, or offscreen flow.

## Minimal working snippet

```c
#include "se_graphics.h"

se_render_clear();
```

## Step-by-step explanation

1. Call the graphics helpers after the correct window, framebuffer, or render buffer is already active.
1. Keep higher-level ownership in modules such as `se_scene`, `se_ui`, or `se_vfx`, and use `se_graphics` only for the low-level state changes that remain.
1. Pair clear/state operations with the correct output target so offscreen passes and window passes do not bleed into each other.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_graphics.md).
1. Continue with the [Graphics path page](../path/graphics.md).

</div>

## Common mistakes

- Clearing the wrong target because a framebuffer or render buffer is still bound.
- Treating `se_graphics` as a scene or object ownership layer.
- Repeating low-level state changes every frame when a higher-level module already handles them.

## Related pages

- [Path: Graphics](../path/graphics.md)
- [Visual building blocks overview](../visual-building-blocks/index.md)
- [Module guide: se_framebuffer](se-framebuffer.md)
- [Module guide: se_render_buffer](se-render-buffer.md)
- [Example: hello_text](../examples/default/hello_text.md)
