---
title: se_texture Playbook
summary: Texture loading progression from one file to runtime-friendly handle management.
prerequisites:
  - Texture assets available.
  - Shader/material path already configured.
---

# se_texture Playbook

## When to use this

Use this when texture data should stay in explicit handles with clear ownership and wrap behavior.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_texture` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_texture/step1_minimal.c"
```

Key API calls:
- `se_texture_load`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_texture/step2_core.c"
```

Key API calls:
- `se_texture_load`
- `se_texture_load_from_memory`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_texture/step3_interactive.c"
```

Key API calls:
- `se_texture_load`
- `se_texture_load_from_memory`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_texture/step4_complete.c"
```

Key API calls:
- `se_texture_load`
- `se_texture_load_from_memory`
- `se_texture_destroy`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [loader se loader](../playbooks/loader-se-loader.md)
- Run and compare with: [Linked example](../examples/default/model_viewer.md)

## Related pages

- [Module guide](../module-guides/se-texture.md)
- [API: se_texture.h](../api-reference/modules/se_texture.md)
- [Example: model_viewer](../examples/default/model_viewer.md)
- [Example: vfx_emitters](../examples/advanced/vfx_emitters.md)
