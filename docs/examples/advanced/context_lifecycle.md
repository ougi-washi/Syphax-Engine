---
title: context_lifecycle
summary: Walkthrough page for context_lifecycle.
prerequisites:
  - Build toolchain and resources available.
---

# context_lifecycle

> Scope: advanced

![context_lifecycle preview](../../assets/img/examples/advanced/context_lifecycle.svg)

*Caption: representative preview panel for `context_lifecycle`.*

## Goal

Validate resource ownership and context teardown reports.


## Learning path

- This example corresponds to [Window path page](../../path/window.md) Step 4.
- This example corresponds to [Framebuffer path page](../../path/framebuffer.md) Step 4.
- Next: apply one change from the linked path step and rerun this target.
## Controls

- No runtime controls. Uses validation output in console.

## Build command

```bash
./build.sh context_lifecycle
```

## Run command

```bash
./bin/context_lifecycle
```

## Edits to try

1. Add one resource type to setup.
1. Change expected counts and verify failure path.
1. Run on different backends and compare output.

## Related API links

- [Path: Window](../../path/window.md)
- [Module guide: se_window](../../module-guides/se-window.md)
- [Module guide: se_framebuffer](../../module-guides/se-framebuffer.md)
- [API: se_window.h](../../api-reference/modules/se_window.md)
