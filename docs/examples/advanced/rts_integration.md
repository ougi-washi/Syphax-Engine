---
title: rts_integration
summary: Reference for rts_integration example.
prerequisites:
  - Build toolchain and resources available.
---

# rts_integration

> Scope: advanced

<img src="../../../assets/img/examples/advanced/rts_integration.png" alt="rts_integration preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/advanced/rts_integration.svg';">


## Goal

Integration-scale runtime combining scene, UI, debug, and simulation patterns.


## Learning path

- This example corresponds to [Simulation path page](../../path/simulation.md) Step 4.
- This example corresponds to [Debug path page](../../path/debug.md) Step 4.

## Controls

- Use startup control map printed by target
- Esc: quit

## Build command

```bash
./build.sh rts_integration
```

## Run command

```bash
./bin/rts_integration
```

## Internal flow

- This target runs a full integration loop across scene, UI, simulation, and debug systems.
- Runtime flags control autotest/perf/system-tracking behavior without changing source code.
- Frame diagnostics and subsystem timings are emitted to validate integration behavior.

## Related API links

- [Path: Simulation](../../path/simulation.md)
- [Module guide: se_scene](../../module-guides/se-scene.md)
- [Module guide: se_debug](../../module-guides/se-debug.md)
- [API: se_scene.h](../../api-reference/modules/se_scene.md)
