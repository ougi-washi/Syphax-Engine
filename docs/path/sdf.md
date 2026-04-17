---
title: SDF
summary: SDF graph flow from first visible primitive to the full example scene with shared lighting and shadow tuning.
prerequisites:
  - A 3D runtime target is available.
  - Basic signed-distance field concepts are understood.
---

# SDF

If you are not sure this is the right SDF page, start with the [SDF overview](../sdf/index.md). This page is the guided step-by-step walkthrough, not the shortest overview.

## When to use this

Use this when signed-distance content should be assembled from reusable `se_sdf` objects, shared lights or noise should be attached by handle, and the same scene should scale from a single visible primitive to a full rendered example.

## Quick start

```c
se_sdf_create(...);
se_sdf_render_to_window(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest renderable setup: one sphere, one camera, one window loop.

<img src="../../assets/img/path/sdf/step1.png" alt="SDF Step 1 capture showing a single shaded sphere">

```c
--8<-- "snippets/se_sdf/step1_minimal.c"
```

Key API calls:

- `se_context_create`
- `se_window_create`
- `se_camera_create`
- `se_sdf_create`
- `se_sdf_render_to_window`
- `se_sdf_destroy`

## Step 2: Add Core Feature

Add the scene graph structure so the example starts rendering the same sphere-plus-ground layout used by `examples/sdf.c`.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

<img src="../../assets/img/path/sdf/step2.png" alt="SDF Step 2 capture showing the sphere composed with a ground plane">

```c
--8<-- "snippets/se_sdf/step2_core.c"
```

Key API calls:

- `se_sdf_create`
- `se_sdf_add_child`
- `se_sdf_destroy`

## Step 3: Interactive / Tunable

Add tunable surface noise plus a point light so the same scene starts showing the interactive material and lighting controls exposed by the example.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

<img src="../../assets/img/path/sdf/step3.png" alt="SDF Step 3 capture showing noisy surface detail and a point light">

```c
--8<-- "snippets/se_sdf/step3_interactive.c"
```

Key API calls:

- `se_sdf_create`
- `se_sdf_add_noise`
- `se_sdf_noise_set_frequency`
- `se_sdf_noise_set_offset`
- `se_sdf_add_point_light`
- `se_sdf_point_light_set_position`
- `se_sdf_set_position`

## Step 4: Complete Practical Demo

Complete the flow with the full `sdf` example scene: shared lights, stylized shadow tuning, and the final frame loop.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

<img src="../../assets/img/path/sdf/step4.png" alt="SDF Step 4 capture showing the complete SDF example scene">

```c
--8<-- "snippets/se_sdf/step4_complete.c"
```

Key API calls:

- `se_sdf_create`
- `se_sdf_add_directional_light`
- `se_sdf_point_light_set_radius`
- `se_sdf_directional_light_set_direction`
- `se_sdf_render_to_window`
- `se_sdf_destroy`

## Common mistakes

- Adding lights to a child when the intent is to light the full SDF scene. Parent lights affect every child beneath that parent.
- Mixing `se_sdf_shading.smoothness` with `se_sdf_shadow.softness`. `smoothness` shapes the lit band on the material, while `softness` controls shadow falloff.
- Destroying child SDFs manually after attaching them to a parent. Destroy the parent scene when you want to release the full subtree.

## Next

- Next step: [Physics](physics.md)
- Run and compare with: [Linked example](../examples/default/sdf.md)

## Related pages

- [SDF overview](../sdf/index.md)
- [Module guide: se-camera](../module-guides/se-camera.md)
- [API: se_sdf.h](../api-reference/modules/se_sdf.md)
- [Example: sdf](../examples/default/sdf.md)
- [Path: Camera](camera.md)
