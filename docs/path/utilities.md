---
title: Utilities
summary: Utility-focused progression for math, results, extension checks, and quad helpers.
prerequisites:
  - General C structs and enums familiarity.
  - Need reusable utility patterns across modules.
---

# Utilities

## When to use this

Use this when foundational utility APIs are needed to keep higher-level module code concise and safe.

## Quick start

```c
se_box_2d_make(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_math + se_defines + se_ext + se_quad` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_math_defines_ext_quad/step1_minimal.c"
```

Key API calls:

- `se_box_2d_make`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_math_defines_ext_quad/step2_core.c"
```

Key API calls:

- `se_box_2d_make`
- `se_set_last_error`
- `se_result_str`
- `se_get_last_error`
- `se_ext_is_supported`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_math_defines_ext_quad/step3_interactive.c"
```

Key API calls:

- `se_box_2d_make`
- `se_set_last_error`
- `se_result_str`
- `se_get_last_error`
- `se_ext_is_supported`
- `se_quad_2d_create`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_math_defines_ext_quad/step4_complete.c"
```

Key API calls:

- `se_box_2d_make`
- `se_set_last_error`
- `se_result_str`
- `se_get_last_error`
- `se_ext_is_supported`
- `se_quad_2d_create`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [Backend](backend.md)
- Run and compare with: [Linked example](../examples/advanced/array_handles.md)

## Related pages

- [Module guide](../module-guides/index.md)
- [API: se_math.h](../api-reference/modules/se_math.md)
- [API: se_defines.h](../api-reference/modules/se_defines.md)
- [API: se_ext.h](../api-reference/modules/se_ext.md)
- [API: se_quad.h](../api-reference/modules/se_quad.md)
- [Example: array_handles](../examples/advanced/array_handles.md)
- [Example: scene2d_instancing](../examples/default/scene2d_instancing.md)
