---
title: se_texture
summary: Texture loading, generated texture creation, and optional CPU-side sampling helpers.
prerequisites:
  - Public header include path available.
---

# se_texture

## When to use this

Use `se_texture` when a feature needs image loading, generated texture handles, wrap control, or CPU-side texture sampling.

## Minimal working snippet

```c
#include "se_texture.h"

se_texture_handle texture = se_texture_load(path, SE_CLAMP);
```

## Step-by-step explanation

1. Load textures from packaged resources or create them from memory/generated data depending on where the pixels come from.
1. Pass the resulting handle into scenes, models, UI, VFX, SDF, or custom shader flows instead of exposing raw backend texture IDs.
1. Use sampling helpers only when a feature truly needs CPU-side inspection of texture content.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_texture.md).
1. Continue with the [Texture path page](../path/texture.md).

</div>

## Common mistakes

- Using the wrong resource scope macro or path root when loading packaged assets.
- Treating backend texture IDs as public ownership instead of using texture handles.
- Using CPU-side texture sampling in hot paths when the texture is only needed on the GPU.

## Related pages

- [Path: Texture](../path/texture.md)
- [Visual building blocks overview](../visual-building-blocks/index.md)
- [Module guide: se_noise](se-noise.md)
- [Example: model_viewer](../examples/default/model_viewer.md)
- [Example: noise_texture](../examples/advanced/noise_texture.md)
