---
title: se_vfx Playbook
summary: Emitter lifecycle from first spawn to tunable burst-driven effects.
prerequisites:
  - Texture/model assets available.
  - Frame loop and timing basics ready.
---

# se_vfx Playbook

## When to use this

Use this when particle behavior should be data-driven and tuned live without rewriting update loops.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_vfx` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_vfx/step1_minimal.c"
```

Key API calls:

- `se_vfx_2d_create`
- `se_vfx_2d_add_emitter`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_vfx/step2_core.c"
```

Key API calls:

- `se_vfx_2d_create`
- `se_vfx_2d_add_emitter`
- `se_vfx_2d_emitter_start`
- `se_vfx_2d_emitter_set_spawn`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_vfx/step3_interactive.c"
```

Key API calls:

- `se_vfx_2d_create`
- `se_vfx_2d_add_emitter`
- `se_vfx_2d_emitter_start`
- `se_vfx_2d_emitter_set_spawn`
- `se_vfx_2d_emitter_burst`
- `se_vfx_2d_emitter_set_blend_mode`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_vfx/step4_complete.c"
```

Key API calls:

- `se_vfx_2d_create`
- `se_vfx_2d_add_emitter`
- `se_vfx_2d_emitter_start`
- `se_vfx_2d_emitter_set_spawn`
- `se_vfx_2d_emitter_burst`
- `se_vfx_2d_emitter_set_blend_mode`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se curve](../playbooks/se-curve.md)
- Run and compare with: [Linked example](../examples/advanced/vfx_emitters.md)

## Related pages

- [Module guide](../module-guides/se-vfx.md)
- [API: se_vfx.h](../api-reference/modules/se_vfx.md)
- [Example: vfx_emitters](../examples/advanced/vfx_emitters.md)
