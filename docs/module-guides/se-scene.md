---
title: se_scene
summary: Scene2D and Scene3D ownership, drawing, picking, and JSON helpers.
prerequisites:
  - Public header include path available.
---

# se_scene

## When to use this

Use `se_scene` when objects need a shared draw target, a shared camera, picking, instance management, or scene-level JSON import/export.

## Minimal working snippet

```c
#include "se_scene.h"

se_scene_2d_draw(scene, window);
```

## Step-by-step explanation

1. Create a 2D or 3D scene to own the output surface and the list of referenced objects.
1. Add objects or models to the scene, then let scene draw calls handle the render pass instead of manually iterating every object yourself.
1. Use the scene APIs for picking, framebuffer output, and JSON save/load when those behaviors belong to the scene as a whole.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_scene.md).
1. Continue with the [Scene path page](../path/scene.md).

</div>

## Common mistakes

- Destroying a scene without deciding whether referenced objects, models, or shaders should also be destroyed.
- Reimplementing pick or output-surface logic at app level when the scene module already owns it.
- Treating a scene as gameplay state instead of as a rendering/content container.

## Related pages

- [Path: Scene](../path/scene.md)
- [Module guide: se_camera](se-camera.md)
- [Module guide: se_framebuffer](se-framebuffer.md)
- [Example: scene2d_click](../examples/default/scene2d_click.md)
- [Example: scene3d_json](../examples/default/scene3d_json.md)
