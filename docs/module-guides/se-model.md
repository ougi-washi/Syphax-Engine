---
title: se_model
summary: OBJ loading, transform helpers, and CPU/GPU mesh data control.
prerequisites:
  - Public header include path available.
---

# se_model

## When to use this

Use `se_model` when mesh data should be loaded from OBJ, transformed as a model, or managed with explicit CPU/GPU residency rules.

## Minimal working snippet

```c
#include "se_model.h"

se_model_handle model = se_model_load_obj_simple(obj_path, vertex_shader_path, fragment_shader_path);
```

## Step-by-step explanation

1. Pick the load path and mesh data flags based on whether runtime code still needs CPU mesh access after upload.
1. Use model transforms and scene integration for normal rendering, or render a model directly when the scene layer is unnecessary.
1. Discard CPU mesh data once the runtime no longer needs it, but keep the handle alive as long as any scene or emitter still references it.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_model.md).
1. Continue with the [Model path page](../path/model.md).

</div>

## Common mistakes

- Keeping CPU mesh data for large assets when only GPU draw data is needed.
- Destroying a model while a scene object or VFX emitter still references it.
- Using model loading when the real task is glTF scene import through the loader modules.

## Related pages

- [Path: Model](../path/model.md)
- [3D and camera overview](../three-d-and-camera/index.md)
- [Module guide: loader/se_loader](loader-se-loader.md)
- [Example: model_viewer](../examples/default/model_viewer.md)
- [Example: gltf_roundtrip](../examples/advanced/gltf_roundtrip.md)
