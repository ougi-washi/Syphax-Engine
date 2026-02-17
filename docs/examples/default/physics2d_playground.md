---
title: physics2d_playground
summary: Walkthrough page for physics2d_playground.
prerequisites:
  - Build toolchain and resources available.
---

# physics2d_playground

<picture>
  <source srcset="../../../assets/img/examples/default/physics2d_playground.png" type="image/png">
  <img src="../../../assets/img/examples/default/physics2d_playground.svg" alt="physics2d_playground preview image">
</picture>

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Step a 2D physics world and sync body positions to 2D render objects.


## Learning path

- This example corresponds to [Physics path page](../../path/physics.md) Step 3.
- Next: apply one change from the linked path step and rerun this target.
## Controls

- Space: push all balls upward
- Esc: quit

## Build command

```bash
./build.sh physics2d_playground
```

## Run command

```bash
./bin/physics2d_playground
```

## Edits to try

1. Increase body count.
1. Adjust gravity magnitude.
1. Tune impulse strength.

## Related API links

- [Path: Physics](../../path/physics.md)
- [Module guide: se_physics](../../module-guides/se-physics.md)
- [API: se_physics.h](../../api-reference/modules/se_physics.md)
