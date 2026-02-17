---
title: GLTF
summary: GLTF/GLB parsing progression from load to export and cleanup.
prerequisites:
  - Model asset path available.
  - Need explicit import/export control.
---

# GLTF

## When to use this

Use this when you need direct access to parsed GLTF data and deterministic export operations.

## Quick start

```c
se_gltf_load(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `loader/se_gltf` with explicit handles and one safe call path.

```c
--8<-- "snippets/loader_se_gltf/step1_minimal.c"
```

Key API calls:

- `se_gltf_load`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/loader_se_gltf/step2_core.c"
```

Key API calls:

- `se_gltf_load`
- `se_gltf_write`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/loader_se_gltf/step3_interactive.c"
```

Key API calls:

- `se_gltf_load`
- `se_gltf_write`
- `se_set_last_error`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/loader_se_gltf/step4_complete.c"
```

Key API calls:

- `se_gltf_load`
- `se_gltf_write`
- `se_set_last_error`
- `se_gltf_free`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [Render Buffer](render-buffer.md)
- Run and compare with: [Linked example](../examples/advanced/gltf_roundtrip.md)

## Related pages

- [Module guide](../module-guides/loader-se-gltf.md)
- [API: loader/se_gltf.h](../api-reference/modules/loader_se_gltf.md)
- [Example: gltf_roundtrip](../examples/advanced/gltf_roundtrip.md)
