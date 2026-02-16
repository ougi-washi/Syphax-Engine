---
title: se_curve Playbook
summary: Progress from basic keyframes to tunable interpolation for runtime animation values.
prerequisites:
  - Need value interpolation over time.
  - Basic math/vector types understood.
---

# se_curve Playbook

## When to use this

Use this when scalar/vector tracks must be authorable as keys and evaluated deterministically.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_curve` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_curve/step1_minimal.c"
```

Key API calls:
- `se_curve_float_init`
- `se_curve_float_add_key`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_curve/step2_core.c"
```

Key API calls:
- `se_curve_float_init`
- `se_curve_float_add_key`
- `se_curve_float_sort`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_curve/step3_interactive.c"
```

Key API calls:
- `se_curve_float_init`
- `se_curve_float_add_key`
- `se_curve_float_sort`
- `se_curve_float_eval`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_curve/step4_complete.c"
```

Key API calls:
- `se_curve_float_init`
- `se_curve_float_add_key`
- `se_curve_float_sort`
- `se_curve_float_eval`
- `se_curve_float_clear`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se sdf](../playbooks/se-sdf.md)
- Run and compare with: [Linked example](../examples/advanced/vfx_emitters.md)

## Related pages

- [Module guide](../module-guides/se-curve.md)
- [API: se_curve.h](../api-reference/modules/se_curve.md)
- [Example: vfx_emitters](../examples/advanced/vfx_emitters.md)
