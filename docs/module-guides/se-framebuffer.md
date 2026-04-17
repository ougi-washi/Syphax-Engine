---
title: se_framebuffer
summary: Offscreen color/depth framebuffer handles and target management APIs.
prerequisites:
  - Public header include path available.
---

# se_framebuffer

## When to use this

Use `se_framebuffer` when a render pass needs an offscreen target with engine-managed color and depth attachments.

## Minimal working snippet

```c
#include "se_framebuffer.h"

se_framebuffer_handle fb = se_framebuffer_create(&size);
se_framebuffer_bind(fb);
se_framebuffer_unbind(fb);
```

## Step-by-step explanation

1. Create a framebuffer sized for the render pass you want to isolate.
1. Bind it for the offscreen pass, then unbind it before drawing the result back into a window or another pass.
1. Query texture IDs only when another rendering layer needs the attachment output explicitly.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_framebuffer.md).
1. Continue with the [Framebuffer path page](../path/framebuffer.md).

</div>

## Common mistakes

- Forgetting to unbind before the next window-space draw pass.
- Reaching for raw backend framebuffer objects instead of using the engine handle.
- Assuming a framebuffer resizes automatically when the owning scene or effect does not manage that for you.

## Related pages

- [Path: Framebuffer](../path/framebuffer.md)
- [Visual building blocks overview](../visual-building-blocks/index.md)
- [Module guide: se_render_buffer](se-render-buffer.md)
- [Module guide: se_scene](se-scene.md)
- [Example: context_lifecycle](../examples/advanced/context_lifecycle.md)
