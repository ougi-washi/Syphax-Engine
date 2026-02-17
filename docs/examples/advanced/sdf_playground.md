---
title: sdf_playground
summary: Walkthrough page for sdf_playground.
prerequisites:
  - Build toolchain and resources available.
---

# sdf_playground

> Scope: advanced

<img src="../../../assets/img/examples/advanced/sdf_playground.png" alt="sdf_playground preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/advanced/sdf_playground.svg';">

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Build SDF scene graph primitives and render with stylized shading.


## Learning path

- This example corresponds to [SDF path page](../../path/sdf.md) Step 4.
- Next: apply one change from the linked path step and rerun this target.
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

## Edits to try

1. Add a second primitive type.
1. Modify stylized band levels.
1. Adjust camera align padding.

## Related API links

- [Path: SDF](../../path/sdf.md)
- [API: se_sdf.h](../../api-reference/modules/se_sdf.md)
- [Module guide: se_camera](../../module-guides/se-camera.md)
- [Glossary: scene](../../glossary/terms.md#scene)
