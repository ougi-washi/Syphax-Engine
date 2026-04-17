---
title: editor_sdf
summary: Reference for editor_sdf example.
prerequisites:
  - Build toolchain and resources available.
---

# editor_sdf

> Scope: advanced

<img src="../../../assets/img/examples/advanced/editor_sdf.svg" alt="editor_sdf preview image">

## Goal

Inspect and edit an SDF scene from the keyboard, including transforms, lights, noise, and JSON save/load.

## Learning path

- This example extends the [SDF path page](../../path/sdf.md) with runtime editing and scene round-tripping.
- This example uses the generic editor data model from the [Utilities path page](../../path/utilities.md).

## Controls

- j/k: select item or property
- h/l: switch items and properties panes
- Enter or i: edit selected property text
- +/-: nudge selected numeric property
- [ or ]: switch active component on vector values
- M: enter move mode for the selected position property
- X, Y, Z: choose move axis
- Shift: coarse step
- Alt: fine step
- F: focus selection
- WASD + QE: pan camera
- U/O: yaw camera
- Y/P: pitch camera
- Z/X: zoom camera
- R: reset camera
- Ctrl+S: save JSON
- Ctrl+O: load JSON
- Ctrl+Q: quit

## Build command

```bash
./build.sh editor_sdf
```

## Run command

```bash
./bin/editor_sdf
```

## Internal flow

- The example loads one JSON scene into both `s_json` and a runtime `se_sdf` tree so the editor can keep source data and live handles aligned.
- Numeric edits for transforms, shading, shadow, lights, and noise route through runtime setters so live uniforms update without rebuilding the whole shader.
- Structural edits such as shape type or operation kind still rewrite JSON and rebuild, while `se_editor` metadata drives labels, component names, and keyboard-only move mode.

## Related API links

- [Source code: `examples/advanced/editor_sdf.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/editor_sdf.c)
- [Path: SDF](../../path/sdf.md)
- [Path: Utilities](../../path/utilities.md)
- [API: se_editor.h](../../api-reference/modules/se_editor.md)
- [API: se_sdf.h](../../api-reference/modules/se_sdf.md)
