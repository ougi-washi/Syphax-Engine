---
title: se_simulation
summary: Deterministic simulation entity/component/event APIs.
prerequisites:
  - Public header include path available.
---

# se_simulation


## When to use this

Use `se_simulation` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_simulation.h"
se_simulation_step(sim, 1u);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_simulation.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive Playbook](../playbooks/se-simulation.md)
- [API module page](../api-reference/modules/se_simulation.md)
- [Example: simulation_intro](../examples/advanced/simulation_intro.md)
- [Example: simulation_advanced](../examples/advanced/simulation_advanced.md)
