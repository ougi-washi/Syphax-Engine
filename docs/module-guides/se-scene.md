---
title: se_scene
summary: Scene2D/Scene3D object and instance APIs.
prerequisites:
  - Public header include path available.
---

# se_scene


## When to use this

Use `se_scene` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_scene.h"
se_scene_3d_draw(scene, window);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_scene.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive Playbook](../playbooks/se-scene.md)
- [API module page](../api-reference/modules/se_scene.md)
- [Example: scene2d_click](../examples/default/scene2d_click.md)
- [Example: scene2d_instancing](../examples/default/scene2d_instancing.md)
