---
title: scene3d_json
summary: Reference for scene3d_json example.
prerequisites:
  - Build toolchain and resources available.
---

# scene3d_json

<img src="../../../assets/img/examples/default/scene3d_json.svg" alt="scene3d_json preview image">

## Goal

Round-trip a 3D scene and object snapshot through JSON, then inspect the loaded scene with a camera.

## Learning path

- This example extends the [Scene path page](../../path/scene.md) with JSON save/load for 3D scenes and object transforms.

## Controls

- Esc: quit

## Build command

```bash
./build.sh scene3d_json
```

## Run command

```bash
./bin/scene3d_json
```

## Internal flow

- A 3D scene is created from one cube model object, and the example saves both object-level JSON and the full scene JSON.
- The object is edited in memory before reload so the object snapshot proves it can restore transform state.
- A fresh scene is loaded from disk, given its own camera setup, and then drawn in the regular frame loop.

## Related API links

- [Source code: `examples/scene3d_json.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/scene3d_json.c)
- [Path: Scene](../../path/scene.md)
- [Path: Camera](../../path/camera.md)
- [Module guide: se_scene](../../module-guides/se-scene.md)
- [API: se_scene.h](../../api-reference/modules/se_scene.md)
