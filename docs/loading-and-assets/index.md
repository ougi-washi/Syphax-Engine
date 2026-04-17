---
title: Loading And Assets
summary: Short route for choosing between packaged resources, raw glTF parsing, and higher-level loader helpers.
prerequisites:
  - Use links in this section.
---

# Loading And Assets

Use this page when the question is "how do I get data or assets into the engine?" rather than "which render feature do I learn next?"

## Start here

- Use [Start Here: Resource paths](../start-here/resource-paths.md) if the main problem is packaged vs writable paths.
- Use [Path: Loader](../path/loader.md) if you want the guided route into loader helpers.
- Use [Path: GLTF](../path/gltf.md) if the source asset is glTF or GLB.

## Choose the right layer

- I want a model or scene in runtime handles quickly:
  Read [Module guide: loader/se_loader](../module-guides/loader-se-loader.md).
- I want raw glTF data and control over parsing/writing:
  Read [Module guide: loader/se_gltf](../module-guides/loader-se-gltf.md).
- I only need image textures or packaged resource paths:
  Read [Module guide: se_texture](../module-guides/se-texture.md) and [Module guide: se_defines](../module-guides/se-defines.md).

## Good runnable references

- [Example: model_viewer](../examples/default/model_viewer.md)
- [Example: scene3d_json](../examples/default/scene3d_json.md)
- [Example: gltf_roundtrip](../examples/advanced/gltf_roundtrip.md)

## Related pages

- [Path: Loader](../path/loader.md)
- [Path: GLTF](../path/gltf.md)
- [Module guide: loader/se_loader](../module-guides/loader-se-loader.md)
- [Module guide: loader/se_gltf](../module-guides/loader-se-gltf.md)
- [Start Here: Resource paths](../start-here/resource-paths.md)
