---
title: se_scene Playbook
summary: Start with one object and grow to scene-level render flow and instance updates.
prerequisites:
  - Basic rendering target compiled.
  - Comfort with matrices and transforms.
---

# se_scene Playbook

## When to use this

Use this when objects, transforms, and render order need to be managed as reusable scene data.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_scene` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_scene/step1_minimal.c"
```

Key API calls:
- `se_scene_2d_create`
- `se_object_2d_create`
- `se_scene_2d_add_object`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_scene/step2_core.c"
```

Key API calls:
- `se_scene_2d_create`
- `se_object_2d_create`
- `se_scene_2d_add_object`
- `se_object_2d_set_position`
- `se_scene_2d_render_to_screen`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_scene/step3_interactive.c"
```

Key API calls:
- `se_scene_2d_create`
- `se_object_2d_create`
- `se_scene_2d_add_object`
- `se_object_2d_set_position`
- `se_scene_2d_render_to_screen`
- `se_object_2d_get_position`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_scene/step4_complete.c"
```

Key API calls:
- `se_scene_2d_create`
- `se_object_2d_create`
- `se_scene_2d_add_object`
- `se_object_2d_set_position`
- `se_scene_2d_render_to_screen`
- `se_object_2d_get_position`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se graphics](../playbooks/se-graphics.md)
- Run and compare with: [Linked example](../examples/default/scene2d_click.md)

## Related pages

- [Module guide](../module-guides/se-scene.md)
- [API: se_scene.h](../api-reference/modules/se_scene.md)
- [Example: scene2d_click](../examples/default/scene2d_click.md)
- [Example: scene2d_instancing](../examples/default/scene2d_instancing.md)
- [Example: scene3d_orbit](../examples/default/scene3d_orbit.md)
