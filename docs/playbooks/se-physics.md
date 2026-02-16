---
title: se_physics Playbook
summary: Reference-quality progression from fixed-step world setup to a complete mini simulation flow.
prerequisites:
  - Frame loop and dt handling ready.
  - Scene/object transform basics understood.
---

# se_physics Playbook

## When to use this

Use this when you need deterministic body stepping, collision setup, and runtime tuning for motion systems.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_physics` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_physics/step1_minimal.c"
```

Key API calls:
- `se_physics_world_2d_create`
- `se_physics_world_2d_step`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_physics/step2_core.c"
```

Key API calls:
- `se_physics_world_2d_create`
- `se_physics_world_2d_step`
- `se_physics_body_2d_create`
- `se_physics_body_2d_add_aabb`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_physics/step3_interactive.c"
```

Key API calls:
- `se_physics_world_2d_create`
- `se_physics_world_2d_step`
- `se_physics_body_2d_create`
- `se_physics_body_2d_add_aabb`
- `se_physics_body_2d_apply_impulse`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_physics/step4_complete.c"
```

Key API calls:
- `se_physics_world_2d_create`
- `se_physics_world_2d_step`
- `se_physics_body_2d_create`
- `se_physics_body_2d_add_aabb`
- `se_physics_body_2d_apply_impulse`
- `se_physics_body_2d_destroy`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se navigation](../playbooks/se-navigation.md)
- Run and compare with: [Linked example](../examples/default/physics2d_playground.md)

## Related pages

- [Module guide](../module-guides/se-physics.md)
- [API: se_physics.h](../api-reference/modules/se_physics.md)
- [Example: physics2d_playground](../examples/default/physics2d_playground.md)
- [Example: physics3d_playground](../examples/default/physics3d_playground.md)
