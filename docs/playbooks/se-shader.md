---
title: se_shader Playbook
summary: From basic shader loading to tunable uniforms and hot-reload checks.
prerequisites:
  - Shader source files available.
  - Render loop already active.
---

# se_shader Playbook

## When to use this

Use this when rendering behavior should be tuned through explicit uniform/state updates.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_shader` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_shader/step1_minimal.c"
```

Key API calls:

- `se_shader_load`
- `se_shader_use`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_shader/step2_core.c"
```

Key API calls:

- `se_shader_load`
- `se_shader_use`
- `se_shader_set_float`
- `se_shader_set_vec4`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_shader/step3_interactive.c"
```

Key API calls:

- `se_shader_load`
- `se_shader_use`
- `se_shader_set_float`
- `se_shader_set_vec4`
- `se_shader_reload_if_changed`
- `se_shader_get_uniform_location`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_shader/step4_complete.c"
```

Key API calls:

- `se_shader_load`
- `se_shader_use`
- `se_shader_set_float`
- `se_shader_set_vec4`
- `se_shader_reload_if_changed`
- `se_shader_get_uniform_location`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se texture](../playbooks/se-texture.md)
- Run and compare with: [Linked example](../examples/default/scene2d_click.md)

## Related pages

- [Module guide](../module-guides/se-shader.md)
- [API: se_shader.h](../api-reference/modules/se_shader.md)
- [Example: scene2d_click](../examples/default/scene2d_click.md)
- [Example: model_viewer](../examples/default/model_viewer.md)
