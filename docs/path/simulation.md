---
title: Simulation
summary: From first entity spawn to controlled deterministic stepping and reset.
prerequisites:
  - Entity/component vocabulary understood.
  - Need deterministic runtime behavior.
---

# Simulation

## When to use this

Use this when runtime state should be stepped predictably and replayed/debugged with consistent outcomes.

## Quick start

```c
se_simulation_create(...);
se_simulation_entity_create(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_simulation` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_simulation/step1_minimal.c"
```

Key API calls:

- `se_simulation_create`
- `se_simulation_entity_create`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_simulation/step2_core.c"
```

Key API calls:

- `se_simulation_create`
- `se_simulation_entity_create`
- `se_simulation_entity_alive`
- `se_simulation_entity_count`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_simulation/step3_interactive.c"
```

Key API calls:

- `se_simulation_create`
- `se_simulation_entity_create`
- `se_simulation_entity_alive`
- `se_simulation_entity_count`
- `se_simulation_step`
- `se_simulation_get_tick`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_simulation/step4_complete.c"
```

Key API calls:

- `se_simulation_create`
- `se_simulation_entity_create`
- `se_simulation_entity_alive`
- `se_simulation_entity_count`
- `se_simulation_step`
- `se_simulation_get_tick`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [Debug](debug.md)
- Run and compare with: [Linked example](../examples/advanced/simulation_intro.md)

## Related pages

- [Module guide](../module-guides/se-simulation.md)
- [API: se_simulation.h](../api-reference/modules/se_simulation.md)
- [Example: simulation_intro](../examples/advanced/simulation_intro.md)
- [Example: simulation_advanced](../examples/advanced/simulation_advanced.md)
