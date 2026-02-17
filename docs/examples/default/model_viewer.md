---
title: model_viewer
summary: Walkthrough page for model_viewer.
prerequisites:
  - Build toolchain and resources available.
---

# model_viewer

<picture>
  <source srcset="../../assets/img/examples/default/model_viewer.png" type="image/png">
  <img src="../../assets/img/examples/default/model_viewer.svg" alt="model_viewer preview image">
</picture>

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Load and inspect a 3D model with a camera controller.


## Learning path

- This example corresponds to [Model path page](../../path/model.md) Step 3.
- This example corresponds to [Loader path page](../../path/loader.md) Step 2.
- Next: apply one change from the linked path step and rerun this target.
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

- [Path: Model](../../path/model.md)
- [Module guide: se_model](../../module-guides/se-model.md)
- [API: se_model.h](../../api-reference/modules/se_model.md)
