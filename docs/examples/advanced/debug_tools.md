---
title: debug_tools
summary: Walkthrough page for debug_tools.
prerequisites:
  - Build toolchain and resources available.
---

# debug_tools

> Scope: advanced

![debug_tools preview](../../assets/img/examples/advanced/debug_tools.svg)

*Caption: representative preview panel for `debug_tools`.*

## Goal

Enable trace and overlay diagnostics and inspect frame timing lines.

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

- [Path: debug overlay and traces](../../path/debug-overlay-and-traces.md)
- [Module guide: se_debug](../../module-guides/se-debug.md)
- [API: se_debug.h](../../api-reference/modules/se_debug.md)
