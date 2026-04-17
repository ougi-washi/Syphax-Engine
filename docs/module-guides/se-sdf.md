---
title: se_sdf
summary: Signed distance field trees, lighting, noise, JSON, and render helpers.
prerequisites:
  - Public header include path available.
---

# se_sdf

## When to use this

Use `se_sdf` when a feature needs runtime raymarched shapes, boolean composition, procedural noise, lighting, or JSON-backed SDF scenes.

## Minimal working snippet

```c
#include "se_sdf.h"

se_sdf_handle sdf = se_sdf_create(.type = SE_SDF_SPHERE);
se_sdf_render_to_window(sdf, camera, window, 1.0f);
```

## Step-by-step explanation

1. Create a root SDF, then add child shapes, noise layers, and lights to build the composed scene you want to raymarch.
1. Tune transform, shading, and shadow values directly on the runtime handles so changes take effect without rebuilding unrelated systems.
1. Render to a framebuffer or directly to a window, and use the JSON helpers when the SDF tree should be saved, loaded, or edited by tools.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_sdf.md).
1. Start with the [SDF overview](../sdf/index.md), then continue with the [SDF path page](../path/sdf.md) if you want the full walkthrough.

</div>

## Common mistakes

- Mixing `shading.smoothness` with `shadow.softness`; they control different parts of the look.
- Forgetting that SDF noise and light handles are still part of the SDF runtime ownership graph.
- Rebuilding the whole SDF tree when a runtime setter already covers the change you need.

## Related pages

- [SDF overview](../sdf/index.md)
- [Path: SDF](../path/sdf.md)
- [Module guide: se_noise](se-noise.md)
- [Example: sdf](../examples/default/sdf.md)
- [Example: sdf_path_steps](../examples/advanced/sdf_path_steps.md)
- [Example: editor_sdf](../examples/advanced/editor_sdf.md)
