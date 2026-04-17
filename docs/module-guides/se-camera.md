---
title: se_camera
summary: Camera creation, projection setup, target mode, and screen/world conversion helpers.
prerequisites:
  - Public header include path available.
---

# se_camera

## When to use this

Use `se_camera` whenever a view needs projection setup, orbit/target behavior, or screen-to-world conversion for picking and navigation.

## Minimal working snippet

```c
#include "se_camera.h"

se_camera_set_window_aspect(camera, window);
```

## Step-by-step explanation

1. Create the camera and decide early whether the view should be perspective, orthographic, free-look, or target-based.
1. Keep the camera aspect synced to the window or output surface so projection math stays correct.
1. Use the world/screen/ray helpers for picking, focus, orbit cameras, or editor tooling instead of duplicating the math in app code.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_camera.md).
1. Continue with the [Camera path page](../path/camera.md).

</div>

## Common mistakes

- Forgetting to update aspect when the target window size changes.
- Mixing target mode and manual transform logic without deciding which one owns the view.
- Recomputing custom projection math when a camera helper already exists for the conversion.

## Related pages

- [Path: Camera](../path/camera.md)
- [3D and camera overview](../three-d-and-camera/index.md)
- [Module guide: se_scene](se-scene.md)
- [Example: scene3d_orbit](../examples/default/scene3d_orbit.md)
- [Example: sdf](../examples/default/sdf.md)
