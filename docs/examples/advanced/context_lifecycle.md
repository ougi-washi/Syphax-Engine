---
title: context_lifecycle
summary: Reference for context_lifecycle example.
prerequisites:
  - Build toolchain and resources available.
---

# context_lifecycle

> Scope: advanced

<img src="../../../assets/img/examples/advanced/context_lifecycle.svg" alt="context_lifecycle preview image">


## Goal

Validate resource ownership and context teardown reports.


## Learning path

- This example corresponds to [Window path page](../../path/window.md) Step 4.
- This example corresponds to [Framebuffer path page](../../path/framebuffer.md) Step 4.

## Controls

- Non-interactive example. Inspect stdout diagnostics.

## Build command

```bash
./build.sh context_lifecycle
```

## Run command

```bash
./bin/context_lifecycle
```

## Internal flow

- The example allocates multiple windows and resource types under one context owner.
- It snapshots internal resource counts, destroys the context, then reads the destroy report.
- Expected and actual destroy counts are compared to verify full teardown coverage.

## Related API links

- [Path: Window](../../path/window.md)
- [Module guide: se_window](../../module-guides/se-window.md)
- [Module guide: se_framebuffer](../../module-guides/se-framebuffer.md)
- [API: se_window.h](../../api-reference/modules/se_window.md)
