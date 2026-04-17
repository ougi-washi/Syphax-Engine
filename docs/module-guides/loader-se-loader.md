---
title: loader/se_loader
summary: High-level helpers that turn glTF assets into engine handles and scenes.
prerequisites:
  - Public header include path available.
---

# loader/se_loader

## When to use this

Use `loader/se_loader` when the goal is not raw glTF inspection but getting models, textures, scene objects, bounds, or camera-fit data into runtime engine systems quickly.

## Minimal working snippet

```c
#include "loader/se_loader.h"

se_model_handle model = se_gltf_model_load_first(path, NULL);
```

## Step-by-step explanation

1. Use the loader helpers when you already know the asset should become `se_model`, `se_texture`, or `se_scene` data.
1. Reach for `se_gltf_scene_load(...)`, bounds helpers, or camera-fit helpers when the asset should shape a full 3D view and not just one model handle.
1. Keep the raw `se_gltf_asset` only as long as post-load inspection is needed, then free it and manage the derived runtime handles through their owning modules.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/loader_se_loader.md).
1. Continue with the [Loader path page](../path/loader.md).

</div>

## Common mistakes

- Using raw `loader/se_gltf` parsing when a higher-level loader helper already matches the task.
- Forgetting that runtime handles returned by loader helpers still need normal scene/model/texture teardown.
- Loading a scene without following up with camera-fit or navigation helpers when the asset scale is unknown.

## Related pages

- [Path: Loader](../path/loader.md)
- [Path: GLTF](../path/gltf.md)
- [Module guide: loader/se_gltf](loader-se-gltf.md)
- [Module guide: se_model](se-model.md)
- [Example: gltf_roundtrip](../examples/advanced/gltf_roundtrip.md)
