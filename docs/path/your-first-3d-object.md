---
title: Your First 3D Object
summary: Create Scene3D and add model instances.
prerequisites:
  - Start Here pages complete.
---

# Your First 3D Object


## When to use this

Use this concept when implementing `your-first-3d-object` behavior in a runtime target.

## Minimal working snippet

```c
se_scene_3d_handle scene = se_scene_3d_create_for_window(window, 16);
se_model_handle model = se_model_load_obj_simple(path, vs, fs);
se_scene_3d_add_model(scene, model, &s_mat4_identity);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Camera Orbit And Pan](camera-orbit-and-pan.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
