---
title: scene3d_orbit
summary: Reference for scene3d_orbit example.
prerequisites:
  - Build toolchain and resources available.
---

# scene3d_orbit

<img src="../../../assets/img/examples/default/scene3d_orbit.png" alt="scene3d_orbit preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/scene3d_orbit.svg';">


## Goal

Render a 3D grid of cubes and navigate with orbit camera controls.


## Learning path

- This example corresponds to [Camera path page](../../path/camera.md) Step 3.

## Controls

- Hold left mouse + move: orbit
- Mouse wheel: zoom
- R: reset camera
- Esc: quit

## Build command

```bash
./build.sh scene3d_orbit
```

## Run command

```bash
./bin/scene3d_orbit
```

## Internal flow

- The example creates a 3x3 cube instance grid in one 3D scene object.
- Mouse delta and wheel delta drive orbit/dolly camera updates against a fixed pivot.
- `se_scene_3d_draw` renders with the updated camera matrices every frame.

## Related API links

- [Source code: `examples/scene3d_orbit.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/scene3d_orbit.c)
- [Path: Camera](../../path/camera.md)
- [Module guide: se_camera](../../module-guides/se-camera.md)
- [API: se_camera.h](../../api-reference/modules/se_camera.md)
