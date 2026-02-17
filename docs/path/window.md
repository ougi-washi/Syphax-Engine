---
title: Window
summary: Window lifecycle from minimal frame loop to stable runtime pacing.
prerequisites:
  - Start Here pages completed.
  - A target that already builds.
---

# Window

## When to use this

Use this when you need deterministic frame boundaries, input polling, and runtime pacing controls.

## Quick start

```c
se_window_set_exit_key(...);
se_window_should_close(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_window` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_window/step1_minimal.c"
```

Key API calls:

- `se_window_set_exit_key`
- `se_window_should_close`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_window/step2_core.c"
```

Key API calls:

- `se_window_set_exit_key`
- `se_window_should_close`
- `se_window_begin_frame`
- `se_window_end_frame`
- `se_window_poll_events`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_window/step3_interactive.c"
```

Key API calls:

- `se_window_set_exit_key`
- `se_window_should_close`
- `se_window_begin_frame`
- `se_window_end_frame`
- `se_window_poll_events`
- `se_window_set_target_fps`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_window/step4_complete.c"
```

Key API calls:

- `se_window_set_exit_key`
- `se_window_should_close`
- `se_window_begin_frame`
- `se_window_end_frame`
- `se_window_poll_events`
- `se_window_set_target_fps`

## Dedicated render queue behavior

On `desktop_glfw`, `se_window_create` starts the dedicated render thread automatically.

Keep using the canonical frame loop (`se_window_begin_frame` / `se_window_end_frame`): these calls route to `se_render_frame_begin` / `se_render_frame_submit` when the queue is active.

While the queue is active, legacy direct present/context calls (`se_window_set_current_context`, `se_window_present`, `se_window_present_frame`, `se_window_render_screen`) return `SE_RESULT_UNSUPPORTED`.

For explicit sync/telemetry, use:

- `se_render_frame_wait_presented`
- `se_render_frame_get_stats`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.
- Mixing `se_window_end_frame` with manual `se_render_frame_submit` in the same frame.
- Calling legacy present/context APIs while dedicated render-thread mode is active.

## Next

- Next step: [Input](input.md)
- Run and compare with: [Linked example](../examples/default/hello_text.md)

## Related pages

- [Module guide](../module-guides/se-window.md)
- [API: se_window.h](../api-reference/modules/se_window.md)
- [API: se_render_frame.h](../api-reference/modules/se_render_frame.md)
- [API: se_render_thread.h](../api-reference/modules/se_render_thread.md)
- [Example: hello_text](../examples/default/hello_text.md)
- [Example: input_actions](../examples/default/input_actions.md)
