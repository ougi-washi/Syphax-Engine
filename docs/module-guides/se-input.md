---
title: se_input
summary: Raw bindings, named actions, contexts, and input recording helpers.
prerequisites:
  - Public header include path available.
---

# se_input

## When to use this

Use `se_input` when raw key or mouse polling is no longer enough and you want named actions, context stacks, text callbacks, or replayable event data.

## Minimal working snippet

```c
#include "se_input.h"

se_input_handle* input = se_input_create(window, 64);
se_input_tick(input);
```

## Step-by-step explanation

1. Create one input handle per window and define named actions instead of scattering raw key checks through gameplay or tool code.
1. Bind actions into contexts so menus, editor modes, and gameplay can switch ownership cleanly.
1. Call `se_input_tick(...)` once per frame, then read action state, queued events, or recorded/replayed mappings from the same handle.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_input.md).
1. Continue with the [Input path page](../path/input.md).

</div>

## Common mistakes

- Forgetting to call `se_input_tick(...)` every frame.
- Mixing direct `se_window_is_key_*` checks with named actions without a clear ownership rule.
- Saving or loading mappings without deciding whether they belong in packaged resources or writable paths.

## Related pages

- [Path: Input](../path/input.md)
- [Interaction overview](../interaction/index.md)
- [Module guide: se_window](se-window.md)
- [Module guide: se_ui](se-ui.md)
- [Example: input_actions](../examples/default/input_actions.md)
