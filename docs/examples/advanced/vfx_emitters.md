---
title: vfx_emitters
summary: Reference for vfx_emitters example.
prerequisites:
  - Build toolchain and resources available.
---

# vfx_emitters

> Scope: advanced

<img src="../../../assets/img/examples/advanced/vfx_emitters.png" alt="vfx_emitters preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/advanced/vfx_emitters.svg';">


## Goal

Combined 2D and 3D emitters with animated uniform callbacks.


## Learning path

- This example corresponds to [VFX path page](../../path/vfx.md) Step 4.
- This example corresponds to [Curve path page](../../path/curve.md) Step 3.

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

## Internal flow

- 2D and 3D emitters are configured with curve keys for velocity/color/size evolution.
- Optional burst/blend/tick toggles change simulation behavior while emitters stay live.
- Uniform callbacks animate shader inputs per frame before VFX draw submission.

## Related API links

- [Path: VFX](../../path/vfx.md)
- [Module guide: se_vfx](../../module-guides/se-vfx.md)
- [API: se_vfx.h](../../api-reference/modules/se_vfx.md)
