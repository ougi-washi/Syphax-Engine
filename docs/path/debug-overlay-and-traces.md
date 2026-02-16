---
title: Debug Overlay And Traces
summary: Enable debug traces and read frame timing output.
prerequisites:
  - Start Here pages complete.
---

# Debug Overlay And Traces


## When to use this

Use this concept when implementing `debug-overlay-and-traces` behavior in a runtime target.

## Minimal working snippet

```c
se_debug_set_level(SE_DEBUG_LEVEL_TRACE);
se_debug_trace_begin("frame");
se_debug_trace_end("frame");
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Export And Share](export-and-share.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
