---
title: se_noise
summary: Procedural 2D noise texture creation and update APIs.
prerequisites:
  - Public header include path available.
---

# se_noise

## When to use this

Use `se_noise` when textures should be generated procedurally from a noise descriptor instead of loaded from disk.

## Minimal working snippet

```c
#include "se_noise.h"

se_texture_handle texture = se_noise_2d_create(context, &noise);
```

## Step-by-step explanation

1. Fill a `se_noise_2d` descriptor with the algorithm, frequency, offset, scale, and seed that define the generated pattern.
1. Create the texture once or update it in place when a tool or effect changes the descriptor at runtime.
1. Use the resulting texture handle anywhere a normal texture handle is accepted, then destroy it with `se_noise_2d_destroy(...)` when done.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_noise.md).
1. Inspect the [noise_texture example](../examples/advanced/noise_texture.md).

</div>

## Common mistakes

- Forgetting that the noise API still needs a valid engine context because it creates a normal texture handle underneath.
- Recreating textures every frame when `se_noise_update(...)` is enough.
- Treating the generated texture as an effect by itself instead of wiring it into a scene, shader, or SDF flow.

## Related pages

- [Visual building blocks overview](../visual-building-blocks/index.md)
- [Path: Texture](../path/texture.md)
- [Module guide: se_texture](se-texture.md)
- [Module guide: se_sdf](se-sdf.md)
- [Example: noise_texture](../examples/advanced/noise_texture.md)
