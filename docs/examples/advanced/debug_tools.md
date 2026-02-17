---
title: debug_tools
summary: Walkthrough page for debug_tools.
prerequisites:
  - Build toolchain and resources available.
---

# debug_tools

> Scope: advanced

<picture>
  <source srcset="../../assets/img/examples/advanced/debug_tools.png" type="image/png">
  <img src="../../assets/img/examples/advanced/debug_tools.svg" alt="debug_tools preview image">
</picture>

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Enable trace and overlay diagnostics and inspect frame timing lines.


## Learning path

- This example corresponds to [Debug path page](../../path/debug.md) Step 3.
- Next: apply one change from the linked path step and rerun this target.
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

## Edits to try

1. Reduce debug level to INFO.
1. Add one custom trace span.
1. Write timing output to file.

## Related API links

- [Path: Debug](../../path/debug.md)
- [Module guide: se_debug](../../module-guides/se-debug.md)
- [API: se_debug.h](../../api-reference/modules/se_debug.md)
