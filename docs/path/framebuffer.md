---
title: Framebuffer
summary: Lifecycle guide for framebuffer handles, resizing, and texture extraction.
prerequisites:
  - Need offscreen rendering or custom compositing.
  - Render loop already active.
---

# Framebuffer

## When to use this

Use this when an explicit framebuffer object lifecycle is needed for multi-pass rendering.

## Quick start

```c
se_framebuffer_create(...);
se_framebuffer_bind(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_framebuffer` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_framebuffer/step1_minimal.c"
```

Key API calls:

- `se_framebuffer_create`
- `se_framebuffer_bind`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_framebuffer/step2_core.c"
```

Key API calls:

- `se_framebuffer_create`
- `se_framebuffer_bind`
- `se_framebuffer_set_size`
- `se_framebuffer_get_texture_id`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_framebuffer/step3_interactive.c"
```

Key API calls:

- `se_framebuffer_create`
- `se_framebuffer_bind`
- `se_framebuffer_set_size`
- `se_framebuffer_get_texture_id`
- `se_framebuffer_unbind`
- `se_framebuffer_cleanup`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_framebuffer/step4_complete.c"
```

Key API calls:

- `se_framebuffer_create`
- `se_framebuffer_bind`
- `se_framebuffer_set_size`
- `se_framebuffer_get_texture_id`
- `se_framebuffer_unbind`
- `se_framebuffer_cleanup`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [VFX](vfx.md)
- Run and compare with: [Linked example](../examples/advanced/context_lifecycle.md)

## Related pages

- [Module guide](../module-guides/se-framebuffer.md)
- [API: se_framebuffer.h](../api-reference/modules/se_framebuffer.md)
- [Example: context_lifecycle](../examples/advanced/context_lifecycle.md)
