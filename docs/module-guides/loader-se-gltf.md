---
title: loader/se_gltf
summary: Raw glTF parse, write, and asset-lifetime APIs.
prerequisites:
  - Public header include path available.
---

# loader/se_gltf

## When to use this

Use `loader/se_gltf` when you want direct access to raw glTF data such as nodes, materials, images, animations, or buffers before turning it into engine handles.

## Minimal working snippet

```c
#include "loader/se_gltf.h"

se_gltf_asset* asset = se_gltf_load(path, NULL);
se_gltf_free(asset);
```

## Step-by-step explanation

1. Load a `.gltf` or `.glb` file into a CPU-side `se_gltf_asset` when you need to inspect or modify its structure directly.
1. Read or edit asset data in place, or pass the asset to higher-level loader helpers that produce models, textures, or scene objects.
1. Free the asset explicitly with `se_gltf_free(...)` once no caller still needs the parsed raw data.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/loader_se_gltf.md).
1. Continue with the [GLTF path page](../path/gltf.md).

</div>

## Common mistakes

- Assuming `se_gltf_load(...)` creates runtime scene or model handles by itself.
- Forgetting to free the parsed asset after inspection or export work is done.
- Editing raw asset fields while another layer assumes they are immutable.

## Related pages

- [Path: GLTF](../path/gltf.md)
- [Module guide: loader/se_loader](loader-se-loader.md)
- [Example: gltf_roundtrip](../examples/advanced/gltf_roundtrip.md)
