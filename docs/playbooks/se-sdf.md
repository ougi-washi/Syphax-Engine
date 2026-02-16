---
title: se_sdf Playbook
summary: SDF scene graph flow from scene creation to validated transform updates.
prerequisites:
  - SDF concepts and scene graph basics understood.
  - Runtime target with camera controls.
---

# se_sdf Playbook

## When to use this

Use this when signed-distance content should be assembled with reusable scene nodes and validated structure.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_sdf` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_sdf/step1_minimal.c"
```

Key API calls:
- `se_sdf_scene_create`
- `se_sdf_scene_clear`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_sdf/step2_core.c"
```

Key API calls:
- `se_sdf_scene_create`
- `se_sdf_scene_clear`
- `se_sdf_node_create_group`
- `se_sdf_scene_set_root`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_sdf/step3_interactive.c"
```

Key API calls:
- `se_sdf_scene_create`
- `se_sdf_scene_clear`
- `se_sdf_node_create_group`
- `se_sdf_scene_set_root`
- `se_sdf_transform_trs`
- `se_sdf_node_set_transform`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_sdf/step4_complete.c"
```

Key API calls:
- `se_sdf_scene_create`
- `se_sdf_scene_clear`
- `se_sdf_node_create_group`
- `se_sdf_scene_set_root`
- `se_sdf_transform_trs`
- `se_sdf_node_set_transform`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se math defines ext quad](../playbooks/se-math-defines-ext-quad.md)
- Run and compare with: [Linked example](../examples/advanced/sdf_playground.md)

## Related pages

- [Module guide](../module-guides/index.md)
- [API: se_sdf.h](../api-reference/modules/se_sdf.md)
- [Example: sdf_playground](../examples/advanced/sdf_playground.md)
