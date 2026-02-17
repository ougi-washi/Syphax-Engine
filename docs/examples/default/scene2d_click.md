---
title: scene2d_click
summary: Reference for scene2d_click example.
prerequisites:
  - Build toolchain and resources available.
---

# scene2d_click

<img src="../../../assets/img/examples/default/scene2d_click.png" alt="scene2d_click preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/scene2d_click.svg';">


## Goal

Pick an object with pointer input and update shader color on click.


## Learning path

- This example corresponds to [Scene path page](../../path/scene.md) Step 2.

## Controls

- Left click center button: toggle color
- Esc: quit

## Build command

```bash
./build.sh scene2d_click
```

## Run command

```bash
./bin/scene2d_click
```

## Internal flow

- Pointer input is converted to normalized coordinates each click frame.
- `se_scene_2d_pick_object` resolves the clicked object handle against scene bounds.
- On hit, shader state (`u_color`) and local click counters update before the next draw.

## Related API links

- [Path: Scene](../../path/scene.md)
- [Module guide: se_scene](../../module-guides/se-scene.md)
- [API: se_scene.h](../../api-reference/modules/se_scene.md)
