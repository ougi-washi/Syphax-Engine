---
title: debug_tools
summary: Reference for debug_tools example.
prerequisites:
  - Build toolchain and resources available.
---

# debug_tools

> Scope: advanced

<img src="../../../assets/img/examples/advanced/debug_tools.png" alt="debug_tools preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/advanced/debug_tools.svg';">


## Goal

Enable trace and overlay diagnostics and inspect frame timing lines.


## Learning path

- This example corresponds to [Debug path page](../../path/debug.md) Step 3.

## Controls

- Esc: quit

## Build command

```bash
./build.sh debug_tools
```

## Run command

```bash
./bin/debug_tools
```

## Internal flow

- Debug level/category masks are enabled before entering the frame loop.
- Trace spans wrap frame work, and the debug subsystem records per-frame timing buckets.
- On shutdown, trace counts and formatted timing lines are dumped for analysis.

## Related API links

- [Path: Debug](../../path/debug.md)
- [Module guide: se_debug](../../module-guides/se-debug.md)
- [API: se_debug.h](../../api-reference/modules/se_debug.md)
