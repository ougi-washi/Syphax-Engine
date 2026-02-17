---
title: physics3d_playground
summary: Reference for physics3d_playground example.
prerequisites:
  - Build toolchain and resources available.
---

# physics3d_playground

<img src="../../../assets/img/examples/default/physics3d_playground.png" alt="physics3d_playground preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/physics3d_playground.svg';">


## Goal

Run a 3D rigid-body simulation and update instance transforms.


## Learning path

- This example corresponds to [Physics path page](../../path/physics.md) Step 4.

## Controls

- Space: launch all cubes upward
- Esc: quit

## Build command

```bash
./build.sh physics3d_playground
```

## Run command

```bash
./bin/physics3d_playground
```

## Internal flow

- A 3D physics world owns rigid bodies while render instances are tracked by `se_instance_id`.
- `se_physics_world_3d_step` updates body transforms, including ground/body collision resolution.
- Updated transforms are pushed with `se_object_3d_set_instance_transform` before scene draw.

## Related API links

- [Path: Physics](../../path/physics.md)
- [Module guide: se_physics](../../module-guides/se-physics.md)
- [API: se_physics.h](../../api-reference/modules/se_physics.md)
