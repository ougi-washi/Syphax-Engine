---
title: se_camera
summary: Camera projection and camera controller APIs.
prerequisites:
  - Public header include path available.
---

# se_camera


## When to use this

Use `se_camera` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_camera.h"
se_camera_set_aspect_from_window(camera, window);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_camera.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive path page](../path/camera.md)
- [API module page](../api-reference/modules/se_camera.md)
- [Example: scene3d_orbit](../examples/default/scene3d_orbit.md)
