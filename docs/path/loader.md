---
title: Loader
summary: Integrate loader helpers from one asset pointer to scene/model utility flow.
prerequisites:
  - GLTF assets available.
  - Model/texture module basics known.
---

# Loader

## When to use this

Use this when asset loading should map directly to scene/model helpers without custom loader wrappers.

## Quick start

```c
se_gltf_model_load(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `loader/se_loader` with explicit handles and one safe call path.

```c
--8<-- "snippets/loader_se_loader/step1_minimal.c"
```

Key API calls:

- `se_gltf_model_load`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/loader_se_loader/step2_core.c"
```

Key API calls:

- `se_gltf_model_load`
- `se_gltf_texture_load`
- `se_gltf_scene_compute_bounds`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/loader_se_loader/step3_interactive.c"
```

Key API calls:

- `se_gltf_model_load`
- `se_gltf_texture_load`
- `se_gltf_scene_compute_bounds`
- `se_gltf_model_load_first`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/loader_se_loader/step4_complete.c"
```

Key API calls:

- `se_gltf_model_load`
- `se_gltf_texture_load`
- `se_gltf_scene_compute_bounds`
- `se_gltf_model_load_first`
- `se_model_destroy`
- `se_texture_destroy`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [GLTF](gltf.md)
- Run and compare with: [Linked example](../examples/default/model_viewer.md)

## Related pages

- [Module guide](../module-guides/loader-se-loader.md)
- [API: loader/se_loader.h](../api-reference/modules/loader_se_loader.md)
- [Example: model_viewer](../examples/default/model_viewer.md)
- [Example: gltf_roundtrip](../examples/advanced/gltf_roundtrip.md)
