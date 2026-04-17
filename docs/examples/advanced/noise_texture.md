---
title: noise_texture
summary: Reference for noise_texture example.
prerequisites:
  - Build toolchain and resources available.
---

# noise_texture

> Scope: advanced

<img src="../../../assets/img/examples/advanced/noise_texture.svg" alt="noise_texture preview image">

## Goal

Generate several noise textures, sample them in code, and preview them side-by-side in a 2D scene.

## Learning path

- This example complements the [Texture path page](../../path/texture.md) with generated texture content instead of file-loaded images.

## Controls

- Esc: quit

## Build command

```bash
./build.sh noise_texture
```

## Run command

```bash
./bin/noise_texture
```

## Internal flow

- The example builds four `se_noise_2d` descriptors with different algorithms, offsets, scales, and seeds.
- Each generated texture is sampled at fixed UV coordinates so the example validates texture content as data, not just as a picture.
- The textures are then bound onto four preview quads inside one 2D scene for live visual comparison.

## Related API links

- [Source code: `examples/advanced/noise_texture.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/noise_texture.c)
- [Path: Texture](../../path/texture.md)
- [API: se_noise.h](../../api-reference/modules/se_noise.md)
- [API: se_texture.h](../../api-reference/modules/se_texture.md)
