---
title: se_input Playbook
summary: Build action maps, contexts, and tunable input behavior from one small setup.
prerequisites:
  - A valid window handle in your target.
  - Basic frame loop in place.
---

# se_input Playbook

## When to use this

Use this when raw keyboard/mouse state should become named actions that are easy to remap and test.

## Step 1: Minimal Working Project

Build the smallest compileable setup that touches `se_input` with explicit handles and one safe call path.

```c
--8<-- "snippets/se_input/step1_minimal.c"
```

Key API calls:

- `se_input_create`
- `se_input_tick`

## Step 2: Add Core Feature

Add the core runtime feature so the module starts doing useful work every frame or tick.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_input/step2_core.c"
```

Key API calls:

- `se_input_create`
- `se_input_tick`
- `se_input_action_create`
- `se_input_action_get_value`

## Step 3: Interactive / Tunable

Add one interactive or tunable behavior so runtime changes are visible and easy to reason about.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_input/step3_interactive.c"
```

Key API calls:

- `se_input_create`
- `se_input_tick`
- `se_input_action_create`
- `se_input_action_get_value`
- `se_input_action_is_pressed`
- `se_input_set_enabled`

## Step 4: Complete Practical Demo

Complete the flow with cleanup and final integration structure suitable for a real target.

What changed from previous step: this step layers one additional capability without replacing the previous structure, so you can isolate behavior changes quickly.

```c
--8<-- "snippets/se_input/step4_complete.c"
```

Key API calls:

- `se_input_create`
- `se_input_tick`
- `se_input_action_create`
- `se_input_action_get_value`
- `se_input_action_is_pressed`
- `se_input_set_enabled`

## Common mistakes

- Skipping explicit cleanup paths when introducing new handles or resources.
- Changing multiple system parameters at once, which hides root-cause behavior shifts.
- Forgetting to keep update frequency stable when adding runtime tuning logic.

## Next

- Next step: [se ui](../playbooks/se-ui.md)
- Run and compare with: [Linked example](../examples/default/input_actions.md)

## Related pages

- [Module guide](../module-guides/se-input.md)
- [API: se_input.h](../api-reference/modules/se_input.md)
- [Example: input_actions](../examples/default/input_actions.md)
