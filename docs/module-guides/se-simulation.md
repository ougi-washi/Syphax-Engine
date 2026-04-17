---
title: se_simulation
summary: Fixed-tick entity, component, event, system, and snapshot APIs.
prerequisites:
  - Public header include path available.
---

# se_simulation

## When to use this

Use `se_simulation` when state should advance in fixed ticks with explicit entities, registered component types, ordered systems, and queued events.

## Minimal working snippet

```c
#include "se_simulation.h"

se_simulation_step(sim, 1u);
```

## Step-by-step explanation

1. Create the simulation with capacities and a fixed time step that match the expected workload.
1. Register component and event types up front, then add systems that operate on deterministic tick state instead of frame delta guesses.
1. Use the JSON and snapshot helpers when you need persistence, rollback, replay, or tool-facing inspection of simulation state.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_simulation.md).
1. Continue with the [Simulation path page](../path/simulation.md).

</div>

## Common mistakes

- Registering simulation types lazily in unpredictable runtime paths.
- Mixing frame-time logic and fixed-tick logic without deciding which layer owns the state.
- Treating the simulation module as an opinionated gameplay framework instead of a generic engine system.

## Related pages

- [Path: Simulation](../path/simulation.md)
- [Simulation and advanced overview](../simulation-and-advanced/index.md)
- [Module guide: se_worker](se-worker.md)
- [Example: simulation_intro](../examples/advanced/simulation_intro.md)
- [Example: simulation_advanced](../examples/advanced/simulation_advanced.md)
