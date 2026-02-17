---
title: vfx_emitters
summary: Walkthrough page for vfx_emitters.
prerequisites:
  - Build toolchain and resources available.
---

# vfx_emitters

> Scope: advanced

![vfx_emitters preview](../../assets/img/examples/advanced/vfx_emitters.svg)

*Caption: representative preview panel for `vfx_emitters`.*

## Goal

Combined 2D and 3D emitters with animated uniform callbacks.


## Learning path

- This example corresponds to [VFX path page](../../path/vfx.md) Step 4.
- This example corresponds to [Curve path page](../../path/curve.md) Step 3.
- Next: apply one change from the linked path step and rerun this target.
## Controls

- Esc: quit

## Build command

```bash
./build.sh vfx_emitters
```

## Run command

```bash
./bin/vfx_emitters
```

## Edits to try

1. Increase spawn rates.
1. Swap texture wrap mode.
1. Change uniform animation speed.

## Related API links

- [Path: VFX](../../path/vfx.md)
- [Path: particles and VFX](../../path/particles-and-vfx.md)
- [Module guide: se_vfx](../../module-guides/se-vfx.md)
- [API: se_vfx.h](../../api-reference/modules/se_vfx.md)
