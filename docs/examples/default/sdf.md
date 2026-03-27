---
title: sdf
summary: Reference for the sdf example.
prerequisites:
  - Build toolchain and resources available.
---

# sdf

<img src="../../../assets/img/examples/default/sdf.png" alt="sdf preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/sdf.svg';">


## Goal

Compose a small signed-distance scene with shared lights, per-object shading, and stylized shadow bands.


## Learning path

- This example corresponds to [SDF path page](../../path/sdf.md) Step 4.

## Controls

- Esc: quit

## Build command

```bash
./build.sh sdf
```

## Run command

```bash
./bin/sdf
```

## Internal flow

- A parent `se_sdf` scene blends sphere and ground children with smooth union.
- Noise, point-light, and directional-light handles are attached once and then adjusted through setters.
- Each SDF object carries its own `se_sdf_shading`, so diffuse, specular, roughness, and `shadow_smooothness` are uploaded as per-object uniforms during render.

## Related API links

- [Source code: `examples/sdf.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/sdf.c)
- [Path: SDF](../../path/sdf.md)
- [Path: Camera](../../path/camera.md)
- [API: se_sdf.h](../../api-reference/modules/se_sdf.md)
