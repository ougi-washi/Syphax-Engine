---
title: scene2d_json
summary: Reference for scene2d_json example.
prerequisites:
  - Build toolchain and resources available.
---

# scene2d_json

<img src="../../../assets/img/examples/default/scene2d_json.svg" alt="scene2d_json preview image">

## Goal

Round-trip a 2D scene and object snapshot through JSON, then render the loaded result.

## Learning path

- This example extends the [Scene path page](../../path/scene.md) with JSON save/load for 2D scenes and object instances.

## Controls

- Esc: quit

## Build command

```bash
./build.sh scene2d_json
```

## Run command

```bash
./bin/scene2d_json
```

## Internal flow

- A source 2D scene is assembled from a panel object plus an instanced button object with per-instance metadata.
- The example saves both one object snapshot and the full scene snapshot, then loads them back into runtime objects.
- The loaded scene is rendered every frame so the JSON round-trip stays visible instead of remaining a file-only test.

## Related API links

- [Source code: `examples/scene2d_json.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/scene2d_json.c)
- [Path: Scene](../../path/scene.md)
- [Module guide: se_scene](../../module-guides/se-scene.md)
- [API: se_scene.h](../../api-reference/modules/se_scene.md)
