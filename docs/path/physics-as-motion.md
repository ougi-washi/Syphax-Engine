---
title: Physics As Motion
summary: Step physics and sync body transforms to render objects.
prerequisites:
  - Start Here pages complete.
---

# Physics As Motion


## When to use this

Use this concept when implementing `physics-as-motion` behavior in a runtime target.

## Minimal working snippet

```c
se_physics_world_2d* world = se_physics_world_2d_create(&params);
se_physics_world_2d_step(world, dt);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Debug Overlay And Traces](debug-overlay-and-traces.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
