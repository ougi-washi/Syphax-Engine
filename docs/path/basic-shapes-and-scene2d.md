---
title: Basic Shapes And Scene2D
summary: Create Scene2D objects and draw them.
prerequisites:
  - Start Here pages complete.
---

# Basic Shapes And Scene2D


## When to use this

Use this concept when implementing `basic-shapes-and-scene2d` behavior in a runtime target.

## Minimal working snippet

```c
se_scene_2d_handle scene = se_scene_2d_create(&s_vec2(1280.0f, 720.0f), 4);
se_object_2d_handle object = se_object_2d_create(SE_RESOURCE_EXAMPLE("scene2d/button.glsl"), &s_mat3_identity, 0);
se_scene_2d_add_object(scene, object);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown>

## Try this next

1. Next page: [Motion With Time](motion-with-time.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
