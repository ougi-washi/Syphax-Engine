---
title: simulation_intro
summary: Reference for simulation_intro example.
prerequisites:
  - Build toolchain and resources available.
---

# simulation_intro

> Scope: advanced

<img src="../../../assets/img/examples/advanced/simulation_intro.svg" alt="simulation_intro preview image">


## Goal

Deterministic simulation starter with component and event registration.


## Learning path

- This example corresponds to [Simulation path page](../../path/simulation.md) Step 2.

## Controls

- Non-interactive example. Inspect stdout diagnostics.

## Build command

```bash
./build.sh simulation_intro
```

## Run command

```bash
./bin/simulation_intro
```

## Internal flow

- Components/events/systems are registered once, then a single entity is initialized.
- Deterministic event sequences are emitted, stepped, snapshotted, restored, and replayed.
- State equality checks after replay verify deterministic simulation behavior.

## Related API links

- [Source code: `examples/advanced/simulation_intro.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/simulation_intro.c)
- [Path: Simulation](../../path/simulation.md)
- [Module guide: se_simulation](../../module-guides/se-simulation.md)
- [API: se_simulation.h](../../api-reference/modules/se_simulation.md)
- [Glossary: deterministic step](../../glossary/terms.md#deterministic-step)
