---
title: se_render_buffer Playbook
summary: Offscreen render flow from one buffer to configurable post-processing placement.
prerequisites:
  - Framebuffer concepts understood.
  - Post-processing shader path available.
---

# se_render_buffer Playbook

## When to use this

Use this when drawing into intermediate targets and compositing to screen in controlled stages.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_render_buffer` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_render_buffer/step1_minimal.c"
```

Key API calls:
- `se_render_buffer_create`
- `se_render_buffer_bind`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_render_buffer/step2_core.c"
```

Key API calls:
- `se_render_buffer_create`
- `se_render_buffer_bind`
- `se_render_buffer_set_scale`
- `se_render_buffer_set_position`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_render_buffer/step3_interactive.c"
```

Key API calls:
- `se_render_buffer_create`
- `se_render_buffer_bind`
- `se_render_buffer_set_scale`
- `se_render_buffer_set_position`
- `se_render_buffer_unbind`
- `se_render_buffer_cleanup`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_render_buffer/step4_complete.c"
```

Key API calls:
- `se_render_buffer_create`
- `se_render_buffer_bind`
- `se_render_buffer_set_scale`
- `se_render_buffer_set_position`
- `se_render_buffer_unbind`
- `se_render_buffer_cleanup`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se framebuffer](../playbooks/se-framebuffer.md)
- Run and compare with: [Linked example](../examples/advanced/multi_window.md)

## Related pages

- [Module guide](../module-guides/se-render-buffer.md)
- [API: se_render_buffer.h](../api-reference/modules/se_render_buffer.md)
- [Example: multi_window](../examples/advanced/multi_window.md)
