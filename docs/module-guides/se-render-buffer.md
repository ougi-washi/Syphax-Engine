---
title: se_render_buffer
summary: Shader-backed post-process buffers with explicit bind, placement, and cleanup control.
prerequisites:
  - Public header include path available.
---

# se_render_buffer

## When to use this

Use `se_render_buffer` when a render pass needs a post-process-style texture target with an associated fragment shader and placement controls.

## Minimal working snippet

```c
#include "se_render_buffer.h"

se_render_buffer_handle buffer = se_render_buffer_create(640, 360, fragment_shader_path);
se_render_buffer_bind(buffer);
se_render_buffer_unbind(buffer);
```

## Step-by-step explanation

1. Create the render buffer for the size and shader pass you want to own.
1. Bind it for the offscreen draw, then unbind and position or scale the result for presentation.
1. Swap or unset shaders deliberately when the pass changes instead of creating a new post-process path for every small variation.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_render_buffer.md).
1. Continue with the [Render Buffer path page](../path/render-buffer.md).

</div>

## Common mistakes

- Forgetting to unbind the render buffer before the next pass.
- Using a render buffer when a plain framebuffer would already be enough.
- Hiding shader ownership so cleanup order becomes unclear.

## Related pages

- [Path: Render Buffer](../path/render-buffer.md)
- [Visual building blocks overview](../visual-building-blocks/index.md)
- [Module guide: se_framebuffer](se-framebuffer.md)
- [Module guide: se_shader](se-shader.md)
- [Example: multi_window](../examples/advanced/multi_window.md)
