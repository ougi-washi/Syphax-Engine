---
title: Mouse And Keyboard Input
summary: Bind actions and query action states.
prerequisites:
  - Start Here pages complete.
---

# Mouse And Keyboard Input


## When to use this

Use this concept when implementing `mouse-and-keyboard-input` behavior in a runtime target.

## Minimal working snippet

```c
se_input_handle* input = se_input_create(window, 32);
se_input_action_create(input, ACTION_MOVE, "move", 0);
se_input_tick(input);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown>

## Try this next

1. Next page: [Buttons And Ui Basics](buttons-and-ui-basics.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
