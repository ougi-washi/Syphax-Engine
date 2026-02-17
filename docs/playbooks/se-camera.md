---
title: se_camera Playbook
summary: Progress from basic camera setup to tunable orbit and dolly control.
prerequisites:
  - A 3D scene or model test target.
  - Window loop already running.
---

# se_camera Playbook

## When to use this

Use this when the runtime needs predictable view/projection setup and smooth camera interaction.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_camera` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_camera/step1_minimal.c"
```

Key API calls:

- `se_camera_create`
- `se_camera_set_aspect`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_camera/step2_core.c"
```

Key API calls:

- `se_camera_create`
- `se_camera_set_aspect`
- `se_camera_set_perspective`
- `se_camera_get_view_projection_matrix`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_camera/step3_interactive.c"
```

Key API calls:

- `se_camera_create`
- `se_camera_set_aspect`
- `se_camera_set_perspective`
- `se_camera_get_view_projection_matrix`
- `se_camera_orbit`
- `se_camera_dolly`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_camera/step4_complete.c"
```

Key API calls:

- `se_camera_create`
- `se_camera_set_aspect`
- `se_camera_set_perspective`
- `se_camera_get_view_projection_matrix`
- `se_camera_orbit`
- `se_camera_dolly`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se scene](../playbooks/se-scene.md)
- Run and compare with: [Linked example](../examples/default/scene3d_orbit.md)

## Related pages

- [Module guide](../module-guides/se-camera.md)
- [API: se_camera.h](../api-reference/modules/se_camera.md)
- [Example: scene3d_orbit](../examples/default/scene3d_orbit.md)
