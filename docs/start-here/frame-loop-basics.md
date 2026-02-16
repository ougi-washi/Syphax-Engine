---
title: Frame Loop Basics
summary: Use the canonical `begin_frame` and `end_frame` structure.
prerequisites:
  - A valid window handle.
---

# Frame Loop Basics


## When to use this

Use this loop in every runtime target unless a module explicitly requires another sequence.

## Minimal working snippet

```c
while (!se_window_should_close(window)) {
	se_window_begin_frame(window);
	// update
	se_render_clear();
	// draw
	se_window_end_frame(window);
}
```

## Step-by-step explanation

1. Begin frame.
1. Update state and process input.
1. Draw and present.

<div class="next-block" markdown>

## Try this next

1. Next page: [Error handling](error-handling.md).
1. Review [frame loop diagram](../assets/img/frame-loop.svg).

</div>

## Common mistakes

- Calling frame functions in the wrong order.
- Skipping `end_frame` on an early branch.

## Related pages

- [First window](first-window.md)
- [Path: motion with time](../path/motion-with-time.md)
- [API: se_window.h](../api-reference/modules/se_window.md)

![Frame loop flow diagram](../assets/img/frame-loop.svg)

*Caption: one frame cycle from begin to present, then loop.*
