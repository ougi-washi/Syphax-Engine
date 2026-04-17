---
title: sdf_path_steps
summary: Reference for sdf_path_steps example.
prerequisites:
  - Build toolchain and resources available.
---

# sdf_path_steps

> Scope: advanced docs helper

Most users can skip this page. It exists to generate the staged screenshots used by the SDF walkthrough.

<img src="../../../assets/img/examples/advanced/sdf_path_steps.svg" alt="sdf_path_steps preview image">

## Goal

Render the staged SDF scenes used by the SDF path walkthrough captures.

## Learning path

- This target is the capture driver behind the [SDF path page](../../path/sdf.md).
- For normal learning, start with the [SDF overview](../../sdf/index.md) or the runnable [sdf example](../default/sdf.md).

## Controls

- `SE_DOCS_SDF_STEP=1..4`: choose which path step to render
- Esc: quit

## Build command

```bash
./build.sh sdf_path_steps
```

## Run command

```bash
SE_DOCS_SDF_STEP=3 ./bin/sdf_path_steps
```

## Internal flow

- The target reads `SE_DOCS_SDF_STEP` from the environment and picks one of four staged scene configurations.
- Each higher step adds the same capability described on the SDF path page, from one sphere through noise and shared lights.
- This keeps the path images reproducible from code instead of requiring separate scene files or manual editor setup.

## Related API links

- [Source code: `examples/advanced/sdf_path_steps.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/sdf_path_steps.c)
- [SDF overview](../../sdf/index.md)
- [Path: SDF](../../path/sdf.md)
- [API: se_sdf.h](../../api-reference/modules/se_sdf.md)
