---
title: Debug
summary: Instrumentation progression from one trace span to full frame timing visibility.
prerequisites:
  - A looped runtime target available.
  - Need diagnostics for performance or behavior.
---

# Debug

## When to use this

Use this when timing, validation, and runtime traces are needed to isolate regressions quickly.

## Quick start

```c
se_debug_set_level(...);
se_debug_frame_begin(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_debug` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_debug/step1_minimal.c"
```

Key API calls:

- `se_debug_set_level`
- `se_debug_frame_begin`
- `se_debug_trace_begin`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_debug/step2_core.c"
```

Key API calls:

- `se_debug_set_level`
- `se_debug_frame_begin`
- `se_debug_trace_begin`
- `se_debug_trace_end`
- `se_debug_frame_end`
- `se_debug_set_overlay_enabled`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_debug/step3_interactive.c"
```

Key API calls:

- `se_debug_set_level`
- `se_debug_frame_begin`
- `se_debug_trace_begin`
- `se_debug_trace_end`
- `se_debug_frame_end`
- `se_debug_set_overlay_enabled`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_debug/step4_complete.c"
```

Key API calls:

- `se_debug_set_level`
- `se_debug_frame_begin`
- `se_debug_trace_begin`
- `se_debug_trace_end`
- `se_debug_frame_end`
- `se_debug_set_overlay_enabled`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [Model](model.md)
- Run and compare with: [Linked example](../examples/advanced/debug_tools.md)

## Related pages

- [Module guide](../module-guides/se-debug.md)
- [API: se_debug.h](../api-reference/modules/se_debug.md)
- [Example: debug_tools](../examples/advanced/debug_tools.md)
- [Example: rts_integration](../examples/advanced/rts_integration.md)
