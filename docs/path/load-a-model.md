---
title: Load A Model
summary: Load model assets and reuse handles for instances.
prerequisites:
  - Start Here pages complete.
---

# Load A Model


## When to use this

Use this concept when implementing `load-a-model` behavior in a runtime target.

## Minimal working snippet

```c
se_model_handle m = se_model_load_obj_simple(SE_RESOURCE_PUBLIC("models/sphere.obj"), vs, fs);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Particles And Vfx](particles-and-vfx.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
- [Deep dive Playbook](../playbooks/se-model.md)
