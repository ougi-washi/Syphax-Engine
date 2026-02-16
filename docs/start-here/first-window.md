---
title: First Window
summary: Create a window safely and close cleanly.
prerequisites:
  - Build completed at least once.
---

# First Window

<div class="learn-block" markdown>

## What you will learn

- Context and window creation order.
- Null-handle failure checks.
- Exit-key setup.

</div>

## When to use this

Use as the baseline setup in any new target.

## Minimal working snippet

```c
se_context* ctx = se_context_create();
se_window_handle window = se_window_create("App", 1280, 720);
if (window == S_HANDLE_NULL) {
	se_context_destroy(ctx);
	return 1;
}
se_window_set_exit_key(window, SE_KEY_ESCAPE);
```

## Step-by-step explanation

1. Create context before window.
1. Check handle right after creation.
1. Destroy context last.

<div class="next-block" markdown>

## Try this next

1. Next page: [Frame loop basics](frame-loop-basics.md).
1. Compare with [hello_text](../examples/default/hello_text.md).

</div>

## Common mistakes

- Using window APIs on invalid handles.
- Destroying context before module resources.

## Related pages

- [Frame loop basics](frame-loop-basics.md)
- [Error handling](error-handling.md)
- [se_window guide](../module-guides/se-window.md)
