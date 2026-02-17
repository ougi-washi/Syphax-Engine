---
title: model_viewer
summary: Reference for model_viewer example.
prerequisites:
  - Build toolchain and resources available.
---

# model_viewer

<img src="../../../assets/img/examples/default/model_viewer.png" alt="model_viewer preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/model_viewer.svg';">


## Goal

Load and inspect a 3D model with a camera controller.


## Learning path

- This example corresponds to [Model path page](../../path/model.md) Step 3.
- This example corresponds to [Loader path page](../../path/loader.md) Step 2.

## Controls

- Mouse + WASD: camera movement
- R: reset camera
- Esc: quit

## Build command

```bash
./build.sh model_viewer
```

## Run command

```bash
./bin/model_viewer
```

## Internal flow

- Model data is loaded once, then reused through one scene object with multiple instances.
- `se_camera_controller_tick` converts input into camera transform updates each frame.
- `se_scene_3d_draw` walks model instances, binds materials/shaders, and submits the 3D pass.

## Related API links

- [Source code: `examples/model_viewer.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/model_viewer.c)
- [Path: Model](../../path/model.md)
- [Module guide: se_model](../../module-guides/se-model.md)
- [API: se_model.h](../../api-reference/modules/se_model.md)
