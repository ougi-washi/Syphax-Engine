---
title: se_navigation Playbook
summary: Build pathfinding from one grid to robust route and smoothing behavior.
prerequisites:
  - Grid/world coordinate conventions decided.
  - Basic structs and arrays understood.
---

# se_navigation Playbook

## When to use this

Use this when navigation needs deterministic grid operations and explicit control over obstacles/path smoothing.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_navigation` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_navigation/step1_minimal.c"
```

Key API calls:
- `se_navigation_grid_create`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_navigation/step2_core.c"
```

Key API calls:
- `se_navigation_grid_create`
- `se_navigation_path_init`
- `se_navigation_find_path_simple`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_navigation/step3_interactive.c"
```

Key API calls:
- `se_navigation_grid_create`
- `se_navigation_path_init`
- `se_navigation_find_path_simple`
- `se_navigation_grid_set_blocked_rect`
- `se_navigation_smooth_path`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_navigation/step4_complete.c"
```

Key API calls:
- `se_navigation_grid_create`
- `se_navigation_path_init`
- `se_navigation_find_path_simple`
- `se_navigation_grid_set_blocked_rect`
- `se_navigation_smooth_path`
- `se_navigation_path_clear`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se simulation](../playbooks/se-simulation.md)
- Run and compare with: [Linked example](../examples/advanced/navigation_grid.md)

## Related pages

- [Module guide](../module-guides/se-navigation.md)
- [API: se_navigation.h](../api-reference/modules/se_navigation.md)
- [Example: navigation_grid](../examples/advanced/navigation_grid.md)
