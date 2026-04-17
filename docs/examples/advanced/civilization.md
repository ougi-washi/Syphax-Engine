---
title: civilization
summary: Reference for civilization example.
prerequisites:
  - Build toolchain and resources available.
---

# civilization

> Scope: advanced

<img src="../../../assets/img/examples/advanced/civilization.svg" alt="civilization preview image">

## Goal

Build a small block-based city scene and navigate it with orbit, zoom, and target-pan camera controls.

## Learning path

- This example extends the [Camera path page](../../path/camera.md) with a larger 3D navigation surface and target-based movement.

## Controls

- Hold left mouse + move: orbit camera
- Mouse wheel: zoom
- WASD + QE: pan camera target
- Shift: faster movement
- R: reset camera
- Esc: quit

## Build command

```bash
./build.sh civilization
```

## Run command

```bash
./bin/civilization
```

## Internal flow

- The example creates one 3D scene, loads a cube model once, and reuses it through many transformed instances.
- Camera motion updates a target point plus yaw, pitch, and distance, then rebuilds the runtime camera transform each frame.
- Ground tiles and building volumes are all instance-driven, so the scene stays simple while still exercising scene and camera APIs together.

## Related API links

- [Source code: `examples/advanced/civilization.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/civilization.c)
- [Path: Camera](../../path/camera.md)
- [Path: Scene](../../path/scene.md)
- [Module guide: se_camera](../../module-guides/se-camera.md)
- [API: se_camera.h](../../api-reference/modules/se_camera.md)
