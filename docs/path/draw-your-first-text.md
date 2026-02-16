---
title: Draw Your First Text
summary: Create text handle and render a label.
prerequisites:
  - Start Here pages complete.
---

# Draw Your First Text


## When to use this

Use this concept when implementing `draw-your-first-text` behavior in a runtime target.

## Minimal working snippet

```c
se_text_handle* text = se_text_handle_create(0);
se_font_handle font = se_font_load(text, SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
se_text_render(text, font, "Hello", &s_vec2(0.0f, 0.0f), &s_vec2(1.0f, 1.0f), 0.03f);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown>

## Try this next

1. Next page: [Basic Shapes And Scene2D](basic-shapes-and-scene2d.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
