---
title: Buttons And UI Basics
summary: Create buttons and respond to click callbacks.
prerequisites:
  - Start Here pages complete.
---

# Buttons And UI Basics


## When to use this

Use this concept when implementing `buttons-and-ui-basics` behavior in a runtime target.

## Minimal working snippet

```c
se_ui_handle ui = se_ui_create(window, 64);
se_ui_widget_handle root = se_ui_create_root(ui);
se_ui_add_button(root, { .text = "Apply", .size = s_vec2(0.2f, 0.08f) });
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Add Sound](add-sound.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
- [Deep dive Playbook](../playbooks/se-ui.md)
