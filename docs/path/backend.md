---
title: Backend
summary: Inspect runtime backend/portability/capability info before feature selection.
prerequisites:
  - Application startup path available.
  - Need backend-aware behavior gates.
---

# Backend

## When to use this

Use this when runtime feature usage must be gated by actual backend capabilities.

## Quick start

```c
se_get_backend_info(...);
```

Use this tiny call path first, then continue with the four progressive snippets below.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_backend` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_backend/step1_minimal.c"
```

Key API calls:

- `se_get_backend_info`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_backend/step2_core.c"
```

Key API calls:

- `se_get_backend_info`
- `se_get_portability_profile`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_backend/step3_interactive.c"
```

Key API calls:

- `se_get_backend_info`
- `se_get_portability_profile`
- `se_capabilities_get`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_backend/step4_complete.c"
```

Key API calls:

- `se_get_backend_info`
- `se_get_portability_profile`
- `se_capabilities_get`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [Curve](curve.md)
- Run and compare with: [Linked example](../examples/advanced/context_lifecycle.md)

## Related pages

- [Module guide](../module-guides/se-backend.md)
- [API: se_backend.h](../api-reference/modules/se_backend.md)
- [Example: context_lifecycle](../examples/advanced/context_lifecycle.md)
