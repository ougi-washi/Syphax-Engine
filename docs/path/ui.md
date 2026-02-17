---
title: UI
summary: From one UI handle to a tuned, responsive widget update/draw cycle.
prerequisites:
  - Window setup complete.
  - Input module basics understood.
---

# UI

## When to use this

Use this when interaction, layout, and widget lifecycle should stay separate from scene/game logic.

## Quick start

```c
se_ui_create(...);
se_ui_tick(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_ui` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_ui/step1_minimal.c"
```

Key API calls:

- `se_ui_create`
- `se_ui_tick`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_ui/step2_core.c"
```

Key API calls:

- `se_ui_create`
- `se_ui_tick`
- `se_ui_draw`
- `se_ui_mark_layout_dirty`
- `se_ui_mark_visual_dirty`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_ui/step3_interactive.c"
```

Key API calls:

- `se_ui_create`
- `se_ui_tick`
- `se_ui_draw`
- `se_ui_mark_layout_dirty`
- `se_ui_mark_visual_dirty`
- `se_ui_is_dirty`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_ui/step4_complete.c"
```

Key API calls:

- `se_ui_create`
- `se_ui_tick`
- `se_ui_draw`
- `se_ui_mark_layout_dirty`
- `se_ui_mark_visual_dirty`
- `se_ui_is_dirty`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [Text](text.md)
- Run and compare with: [Linked example](../examples/default/ui_basics.md)

## Related pages

- [Module guide](../module-guides/se-ui.md)
- [API: se_ui.h](../api-reference/modules/se_ui.md)
- [Example: ui_basics](../examples/default/ui_basics.md)
- [Example: ui_showcase](../examples/advanced/ui_showcase.md)
