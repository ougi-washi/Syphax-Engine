---
title: scene2d_instancing
summary: Reference for scene2d_instancing example.
prerequisites:
  - Build toolchain and resources available.
---

# scene2d_instancing

<img src="../../../assets/img/examples/default/scene2d_instancing.png" alt="scene2d_instancing preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/scene2d_instancing.svg';">


## Goal

Create multiple 2D instances and animate them with a wave function.


## Learning path

- This example corresponds to [Scene path page](../../path/scene.md) Step 3.

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

## Internal flow

- One 2D object owns many instances; IDs are stored in `s_array` for stable iteration.
- Each frame computes a sine-wave offset and rewrites each instance transform.
- Instance updates avoid object recreation, so animation cost stays in transform writes only.

## Related API links

- [Path: Scene](../../path/scene.md)
- [Path: Simulation](../../path/simulation.md)
- [Module guide: se_scene](../../module-guides/se-scene.md)
- [API: se_scene.h](../../api-reference/modules/se_scene.md)
