---
title: se_physics
summary: Rigid body worlds, shapes, and stepping APIs.
prerequisites:
  - Public header include path available.
---

# se_physics


## When to use this

Use `se_physics` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_physics.h"
se_physics_world_3d_step(world, dt);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_physics.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive Playbook](../playbooks/se-physics.md)
- [API module page](../api-reference/modules/se_physics.md)
- [Example: physics2d_playground](../examples/default/physics2d_playground.md)
- [Example: physics3d_playground](../examples/default/physics3d_playground.md)
