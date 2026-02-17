---
title: simulation_advanced
summary: Walkthrough page for simulation_advanced.
prerequisites:
  - Build toolchain and resources available.
---

# simulation_advanced

> Scope: advanced

<picture>
  <img src="../../../assets/img/examples/advanced/simulation_advanced.svg" alt="simulation_advanced preview image">
</picture>

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Extended simulation with system sequencing and diagnostics.


## Learning path

- This example corresponds to [Simulation path page](../../path/simulation.md) Step 4.
- Next: apply one change from the linked path step and rerun this target.
## Controls

- No runtime controls. Observe diagnostics output.

## Build command

```bash
./build.sh simulation_advanced
```

## Run command

```bash
./bin/simulation_advanced
```

## Edits to try

1. Increase event queue capacity.
1. Add one extra system.
1. Compare snapshot outputs across runs.

## Related API links

- [Path: Simulation](../../path/simulation.md)
- [Module guide: se_simulation](../../module-guides/se-simulation.md)
- [Advanced: simulation_intro](simulation_intro.md)
- [API: se_simulation.h](../../api-reference/modules/se_simulation.md)
