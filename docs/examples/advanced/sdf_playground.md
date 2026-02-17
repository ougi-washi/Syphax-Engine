---
title: sdf_playground
summary: Reference for sdf_playground example.
prerequisites:
  - Build toolchain and resources available.
---

# sdf_playground

> Scope: advanced

<img src="../../../assets/img/examples/advanced/sdf_playground.png" alt="sdf_playground preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/advanced/sdf_playground.svg';">


## Goal

Build SDF scene graph primitives and render with stylized shading.


## Learning path

- This example corresponds to [SDF path page](../../path/sdf.md) Step 4.

## Controls

- Camera controls from controller preset
- Esc: quit

## Build command

```bash
./build.sh sdf_playground
```

## Run command

```bash
./bin/sdf_playground
```

## Internal flow

- SDF primitives are assembled into a scene graph and attached to one SDF renderer.
- Camera + frame descriptors (resolution, time, mouse) are refreshed every loop iteration.
- `se_sdf_renderer_render` evaluates the signed-distance shading pass before present.

## Related API links

- [Source code: `examples/advanced/sdf_playground.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/sdf_playground.c)
- [Path: SDF](../../path/sdf.md)
- [API: se_sdf.h](../../api-reference/modules/se_sdf.md)
- [Module guide: se_camera](../../module-guides/se-camera.md)
- [Glossary: scene](../../glossary/terms.md#scene)
