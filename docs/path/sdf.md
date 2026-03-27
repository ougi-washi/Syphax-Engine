---
title: SDF
summary: SDF graph flow from creation to composition, shared lighting, and rendering.
prerequisites:
  - A 3D runtime target is available.
  - Basic signed-distance field concepts are understood.
---

# SDF

## When to use this

Use this when signed-distance content should be assembled from reusable `se_sdf` objects, shared lights or noise should be attached by handle, and per-object shading should stay easy to tune.

## Quick start

```c
se_sdf_create(...);
se_sdf_add_child(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that creates one SDF object and tears it down cleanly.

```c
--8<-- "snippets/se_sdf/step1_minimal.c"
```

Key API calls:

- `se_sdf_create`
- `se_sdf_destroy`

## Step 2: Add Core Feature

Add the core graph feature so the module starts describing a real scene instead of a single primitive.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_sdf/step2_core.c"
```

Key API calls:

- `se_sdf_create`
- `se_sdf_add_child`
- `se_sdf_destroy`

## Step 3: Interactive / Tunable

Add tunable surface and lighting controls so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

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

Complete the flow with shared lights, stylized shadow tuning, and final frame integration.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_sdf/step4_complete.c"
```

Key API calls:

- `se_sdf_create`
- `se_sdf_add_directional_light`
- `se_sdf_point_light_set_radius`
- `se_sdf_directional_light_set_direction`
- `se_sdf_render`
- `se_sdf_destroy`

## Common mistakes

- Adding lights to a child when the intent is to light the full SDF scene. Parent lights affect every child beneath that parent.
- Treating `shadow_smooothness` like raw shadow blur. It shapes the stylized lit-to-shadow band on that object.
- Destroying child SDFs manually after attaching them to a parent. Destroy the parent scene when you want to release the full subtree.

## Next

- Next step: [Physics](physics.md)
- Run and compare with: [Linked example](../examples/default/sdf.md)

## Related pages

- [API: se_sdf.h](../api-reference/modules/se_sdf.md)
- [Example: sdf](../examples/default/sdf.md)
- [Path: Camera](camera.md)
