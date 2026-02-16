---
title: Particles And VFX
summary: Create emitters and tune particle behavior.
prerequisites:
  - Start Here pages complete.
---

# Particles And VFX


## When to use this

Use this concept when implementing `particles-and-vfx` behavior in a runtime target.

## Minimal working snippet

```c
se_vfx_2d_handle vfx = se_vfx_2d_create(NULL);
se_vfx_emitter_2d_params params = SE_VFX_EMITTER_2D_PARAMS_DEFAULTS;
se_vfx_2d_add_emitter(vfx, &params);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown>

## Try this next

1. Next page: [Physics As Motion](physics-as-motion.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
