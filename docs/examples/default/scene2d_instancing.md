---
title: scene2d_instancing
summary: Walkthrough page for scene2d_instancing.
prerequisites:
  - Build toolchain and resources available.
---

# scene2d_instancing

<img src="../../../assets/img/examples/default/scene2d_instancing.png" alt="scene2d_instancing preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/scene2d_instancing.svg';">

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Create multiple 2D instances and animate them with a wave function.


## Learning path

- This example corresponds to [Scene path page](../../path/scene.md) Step 3.
- Next: apply one change from the linked path step and rerun this target.
## Controls

- Space: pause or resume wave motion
- Esc: quit

## Build command

```bash
./build.sh scene2d_instancing
```

## Run command

```bash
./bin/scene2d_instancing
```

## Edits to try

1. Increase instance count.
1. Change wave speed.
1. Add random phase offsets.

## Related API links

- [Path: Scene](../../path/scene.md)
- [Path: Simulation](../../path/simulation.md)
- [Module guide: se_scene](../../module-guides/se-scene.md)
- [API: se_scene.h](../../api-reference/modules/se_scene.md)
