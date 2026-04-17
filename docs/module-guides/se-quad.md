---
title: se_quad
summary: Raw quad builders for custom 2D, 3D, and instanced draw paths.
prerequisites:
  - Public header include path available.
---

# se_quad

## When to use this

Use `se_quad` when a feature needs a raw quad mesh or instanced quad data structure instead of a higher-level scene object or widget.

## Minimal working snippet

```c
#include "se_quad.h"

se_quad quad = {0};
se_quad_2d_create(&quad, 1);
se_quad_render(&quad, 1);
se_quad_destroy(&quad);
```

## Step-by-step explanation

1. Create a stack or owned `se_quad` value when you need a custom quad-backed draw primitive.
1. Add per-instance matrix buffers when the draw path is instanced rather than one-off.
1. Destroy the quad explicitly because this module uses plain structs, not engine handles.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_quad.md).
1. Compare with the [Utilities path page](../path/utilities.md).

</div>

## Common mistakes

- Forgetting to destroy a quad after creating backend buffers.
- Reaching for raw quad helpers when a `se_scene` object or `se_ui` widget would already solve the problem.
- Treating `se_quad` like a public ownership wrapper around scene objects or models.

## Related pages

- [Path: Utilities](../path/utilities.md)
- [Visual building blocks overview](../visual-building-blocks/index.md)
- [Module guide: se_scene](se-scene.md)
- [Module guide: se_render_buffer](se-render-buffer.md)
