---
title: physics2d_playground
summary: Reference for physics2d_playground example.
prerequisites:
  - Build toolchain and resources available.
---

# physics2d_playground

<img src="../../../assets/img/examples/default/physics2d_playground.png" alt="physics2d_playground preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/physics2d_playground.svg';">


## Goal

Step a 2D physics world and sync body positions to 2D render objects.


## Learning path

- This example corresponds to [Physics path page](../../path/physics.md) Step 3.

## Controls

- Space: push all balls upward
- Esc: quit

## Build command

```bash
./build.sh physics2d_playground
```

## Run command

```bash
./bin/physics2d_playground
```

## Internal flow

- A 2D world is created with one static floor body and several dynamic circle bodies.
- `se_physics_world_2d_step` integrates motion and collision response for the current `delta_time`.
- Post-step, each body position is copied to its render object so scene draw matches physics state.

## Related API links

- [Source code: `examples/physics2d_playground.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/physics2d_playground.c)
- [Path: Physics](../../path/physics.md)
- [Module guide: se_physics](../../module-guides/se-physics.md)
- [API: se_physics.h](../../api-reference/modules/se_physics.md)
