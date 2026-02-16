---
title: Motion With Time
summary: Scale movement by delta time.
prerequisites:
  - Start Here pages complete.
---

# Motion With Time

<div class="learn-block" markdown>

## What you will learn

- The minimum calls that make this concept work.
- How this concept fits in the default path order.
- Which module and API pages to open next.

</div>

## When to use this

Use this concept when implementing `motion-with-time` behavior in a runtime target.

## Minimal working snippet

```c
const f32 dt = (f32)se_window_get_delta_time(window);
pos.x += speed * dt;
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown>

## Try this next

1. Next page: [Mouse And Keyboard Input](mouse-and-keyboard-input.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
