---
title: simulation_advanced
summary: Reference for simulation_advanced example.
prerequisites:
  - Build toolchain and resources available.
---

# simulation_advanced

> Scope: advanced

<img src="../../../assets/img/examples/advanced/simulation_advanced.svg" alt="simulation_advanced preview image">


## Goal

Extended simulation with system sequencing and diagnostics.


## Learning path

- This example corresponds to [Simulation path page](../../path/simulation.md) Step 4.

## Controls

- Non-interactive example. Inspect stdout diagnostics.

## Build command

```bash
./build.sh simulation_advanced
```

## Run command

```bash
./bin/simulation_advanced
```

## Internal flow

- The simulation registers ordered systems for event application and fixed-dt integration.
- Diagnostics report queue usage, pending/ready events, and tick progression.
- Validation paths (including stale-entity checks) confirm robust event routing and replay state.

## Related API links

- [Source code: `examples/advanced/simulation_advanced.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/simulation_advanced.c)
- [Path: Simulation](../../path/simulation.md)
- [Module guide: se_simulation](../../module-guides/se-simulation.md)
- [Advanced: simulation_intro](simulation_intro.md)
- [API: se_simulation.h](../../api-reference/modules/se_simulation.md)
