---
title: model_viewer
summary: Walkthrough page for model_viewer.
prerequisites:
  - Build toolchain and resources available.
---

# model_viewer

![model_viewer preview](../../assets/img/examples/default/model_viewer.svg)

*Caption: representative preview panel for `model_viewer`.*

## Goal

Load and inspect a 3D model with a camera controller.

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

## Edits to try

1. Swap sphere and cube assets.
1. Increase movement speed.
1. Add a second row of instances.

## Related API links

- [Path: load a model](../../path/load-a-model.md)
- [Module guide: se_model](../../module-guides/se-model.md)
- [API: se_model.h](../../api-reference/modules/se_model.md)
