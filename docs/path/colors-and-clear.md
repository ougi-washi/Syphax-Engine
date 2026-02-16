---
title: Colors And Clear
summary: Set background color and clear each frame.
prerequisites:
  - Start Here pages complete.
---

# Colors And Clear


## When to use this

Use this concept when implementing `colors-and-clear` behavior in a runtime target.

## Minimal working snippet

```c
se_render_set_background_color(s_vec4(0.05f, 0.07f, 0.1f, 1.0f));
se_render_clear();
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Draw Your First Text](draw-your-first-text.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
