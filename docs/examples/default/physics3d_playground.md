---
title: physics3d_playground
summary: Walkthrough page for physics3d_playground.
prerequisites:
  - Build toolchain and resources available.
---

# physics3d_playground

![physics3d_playground preview](../../assets/img/examples/default/physics3d_playground.svg)

*Caption: representative preview panel for `physics3d_playground`.*

## Goal

Run a 3D rigid-body simulation and update instance transforms.


## Learning path

- This example corresponds to [se_physics Playbook](../../playbooks/se-physics.md) Step 4.
- Next: apply one change from the linked Playbook step and rerun this target.
## Controls

- Space: launch all cubes upward
- Esc: quit

## Build command

```bash
./build.sh physics3d_playground
```

## Run command

```bash
./bin/physics3d_playground
```

## Edits to try

1. Use non-uniform half extents.
1. Add lateral impulse.
1. Change gravity for slower motion.

## Related API links

- [Playbook: se_physics Playbook](../../playbooks/se-physics.md)
- [Path: physics as motion](../../path/physics-as-motion.md)
- [Module guide: se_physics](../../module-guides/se-physics.md)
- [API: se_physics.h](../../api-reference/modules/se_physics.md)
