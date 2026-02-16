---
title: Camera Orbit And Pan
summary: Orbit around a pivot and dolly with wheel input.
prerequisites:
  - Start Here pages complete.
---

# Camera Orbit And Pan


## When to use this

Use this concept when implementing `camera-orbit-and-pan` behavior in a runtime target.

## Minimal working snippet

```c
se_camera_set_orbit_defaults(camera, window, &pivot, 10.0f);
se_camera_orbit(camera, &pivot, dx, dy, -1.45f, 1.45f);
se_camera_dolly(camera, &pivot, wheel, 3.0f, 26.0f);
```

## Step-by-step explanation

1. Start from a working frame loop.
1. Apply the snippet with your current handles.
1. Verify behavior using one example target.

<div class="next-block" markdown="1">

## Next

1. Next: [Load A Model](load-a-model.md).
1. Try one small parameter edit and observe runtime changes.

</div>

## Common mistakes

- Skipping null-handle checks before calling module APIs.
- Changing multiple systems at once and losing isolation.

## Related pages

- [Examples index](../examples/index.md)
- [Module guides](../module-guides/index.md)
- [API module index](../api-reference/modules/index.md)
- [Deep dive Playbook](../playbooks/se-camera.md)
