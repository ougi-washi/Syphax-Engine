---
title: se_vfx
summary: 2D and 3D emitter containers, tracks, callbacks, and draw passes.
prerequisites:
  - Public header include path available.
---

# se_vfx

## When to use this

Use `se_vfx` when particles or billboards should be driven by emitter configuration instead of hand-updating many small scene objects.

## Minimal working snippet

```c
#include "se_vfx.h"

se_vfx_2d_handle vfx = se_vfx_2d_create(NULL);
se_vfx_2d_tick_window(vfx, window);
se_vfx_2d_render(vfx, window);
se_vfx_2d_draw(vfx, window);
```

## Step-by-step explanation

1. Create a 2D or 3D VFX container sized for the output you want to render into, then add emitters with texture, model, shader, or track settings.
1. Update emitters every frame with `tick` helpers so particles age, spawn, and expire correctly.
1. Render the effect into its output surface, then draw that output back to the window or integrate the resulting texture into a broader render flow.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_vfx.md).
1. Continue with the [VFX path page](../path/vfx.md).

</div>

## Common mistakes

- Adding emitters and then never calling the `tick` / `render` / `draw` steps that make them visible.
- Forgetting that 3D VFX rendering still needs a valid camera for billboard or model-facing work.
- Reimplementing lifetime curves and per-particle tuning in app code when emitter tracks and callbacks already cover it.

## Related pages

- [Path: VFX](../path/vfx.md)
- [VFX overview](../vfx/index.md)
- [Module guide: se_curve](se-curve.md)
- [Module guide: se_texture](se-texture.md)
- [Example: vfx_emitters](../examples/advanced/vfx_emitters.md)
