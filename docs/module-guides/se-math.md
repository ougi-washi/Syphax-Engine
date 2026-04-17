---
title: se_math
summary: Small geometry helpers for boxes, circles, spheres, and intersection tests.
prerequisites:
  - Public header include path available.
---

# se_math

## When to use this

Use `se_math` for the simple geometry tests and transform-derived bounds that sit underneath picking, UI hit tests, physics helpers, and navigation logic.

## Minimal working snippet

```c
#include "se_math.h"

se_box_2d_make(&box, &transform);
se_box_2d_is_inside(&box, &point);
```

## Step-by-step explanation

1. Build the shape or bounds data from the transform or world-space values you already have.
1. Run the smallest intersection or containment test that answers the question instead of pulling in a heavier system.
1. Keep these helpers close to data conversion boundaries so higher-level modules receive already-checked results.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_math.md).
1. Compare with the [Utilities path page](../path/utilities.md).

</div>

## Common mistakes

- Testing against stale transforms instead of rebuilding the box or sphere after movement.
- Mixing 2D and 3D helper types in the same call path.
- Using the math helpers as a substitute for a full navigation or physics layer when the problem is broader.

## Related pages

- [Path: Utilities](../path/utilities.md)
- [Physics overview](../physics/index.md)
- [Module guide: se_navigation](se-navigation.md)
- [Module guide: se_physics](se-physics.md)
- [Example: array_handles](../examples/advanced/array_handles.md)
