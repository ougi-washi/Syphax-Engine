---
title: gltf_roundtrip
summary: Reference for gltf_roundtrip example.
prerequisites:
  - Build toolchain and resources available.
---

# gltf_roundtrip

> Scope: advanced

<img src="../../../assets/img/examples/advanced/gltf_roundtrip.svg" alt="gltf_roundtrip preview image">


## Goal

Load GLTF, export GLB, and validate roundtrip asset counts.


## Learning path

- This example corresponds to [GLTF path page](../../path/gltf.md) Step 4.

## Controls

- Non-interactive example. Inspect stdout diagnostics.

## Build command

```bash
./build.sh gltf_roundtrip
```

## Run command

```bash
./bin/gltf_roundtrip
```

## Internal flow

- `se_gltf_load` parses source assets into in-memory arrays (meshes, nodes, scenes, etc.).
- The loaded asset is written as GLB via `se_gltf_write`, then loaded back again.
- Roundtrip validation compares key array counts to catch serialization mismatches.

## Related API links

- [Path: GLTF](../../path/gltf.md)
- [Module guide: loader/se_gltf](../../module-guides/loader-se-gltf.md)
- [Path: Model](../../path/model.md)
- [API: loader/se_gltf.h](../../api-reference/modules/loader_se_gltf.md)
