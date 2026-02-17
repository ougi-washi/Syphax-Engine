---
title: scene2d_click
summary: Walkthrough page for scene2d_click.
prerequisites:
  - Build toolchain and resources available.
---

# scene2d_click

<picture>
  <source srcset="../../../assets/img/examples/default/scene2d_click.png" type="image/png">
  <img src="../../../assets/img/examples/default/scene2d_click.svg" alt="scene2d_click preview image">
</picture>

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Pick an object with pointer input and update shader color on click.


## Learning path

- This example corresponds to [Scene path page](../../path/scene.md) Step 2.
- Next: apply one change from the linked path step and rerun this target.
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

## Edits to try

1. Add a second clickable object.
1. Change click feedback color values.
1. Display click count in a text label.

## Related API links

- [Path: Scene](../../path/scene.md)
- [Module guide: se_scene](../../module-guides/se-scene.md)
- [API: se_scene.h](../../api-reference/modules/se_scene.md)
