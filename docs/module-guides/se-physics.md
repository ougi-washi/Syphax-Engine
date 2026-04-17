---
title: se_physics
summary: 2D and 3D physics worlds, bodies, shapes, and stepping APIs.
prerequisites:
  - Public header include path available.
---

# se_physics

## When to use this

Use `se_physics` when rigid bodies, collisions, queries, or stepped physical motion belong in the runtime.

## Minimal working snippet

```c
#include "se_physics.h"

se_physics_world_3d_step(world, dt);
```

## Step-by-step explanation

1. Create the 2D or 3D world that matches the simulation space you need, then populate it with bodies and shapes.
1. Step the world with a deliberate time step and pull transformed state back into scene or gameplay systems from the results.
1. Use the world APIs for queries and controlled body updates instead of keeping a second untracked motion model in app code.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_physics.md).
1. Continue with the [Physics path page](../path/physics.md).

</div>

## Common mistakes

- Driving physics with inconsistent timing while the rest of the app assumes a fixed step.
- Keeping duplicate transform state in gameplay objects and physics bodies without a clear sync direction.
- Treating the physics world as a gameplay framework instead of as a reusable simulation primitive.

## Related pages

- [Path: Physics](../path/physics.md)
- [Physics overview](../physics/index.md)
- [Module guide: se_math](se-math.md)
- [Example: physics2d_playground](../examples/default/physics2d_playground.md)
- [Example: physics3d_playground](../examples/default/physics3d_playground.md)
