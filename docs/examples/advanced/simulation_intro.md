---
title: simulation_intro
summary: Walkthrough page for simulation_intro.
prerequisites:
  - Build toolchain and resources available.
---

# simulation_intro

> Scope: advanced

![simulation_intro preview](../../assets/img/examples/advanced/simulation_intro.svg)

*Caption: representative preview panel for `simulation_intro`.*

## Goal

Deterministic simulation starter with component and event registration.

## Controls

- No runtime controls. Observe log output for replay checks.

## Build command

```bash
./build.sh simulation_intro
```

## Run command

```bash
./bin/simulation_intro
```

## Edits to try

1. Add one event type.
1. Change replay sequence.
1. Register one additional component.

## Related API links

- [Module guide: se_simulation](../../module-guides/se-simulation.md)
- [API: se_simulation.h](../../api-reference/modules/se_simulation.md)
- [Glossary: deterministic step](../../glossary/terms.md#deterministic-step)
