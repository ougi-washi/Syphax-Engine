---
title: multi_window
summary: Reference for multi_window example.
prerequisites:
  - Build toolchain and resources available.
---

# multi_window

> Scope: advanced

<img src="../../../assets/img/examples/advanced/multi_window.png" alt="multi_window preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/advanced/multi_window.svg';">


## Goal

Render three windows in one context with per-window clear colors.


## Learning path

- This example corresponds to [Window path page](../../path/window.md) Step 3.
- This example corresponds to [Render Buffer path page](../../path/render-buffer.md) Step 2.

## Controls

- Esc on main window: quit all

## Build command

```bash
./build.sh multi_window
```

## Run command

```bash
./bin/multi_window
```

## Internal flow

- Three windows are created in one context, each with its own active GL context state.
- The loop updates each window, switches current context, clears, then presents per window.
- Exit is coordinated through the main window close condition, then context teardown releases all.

## Related API links

- [Source code: `examples/advanced/multi_window.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/multi_window.c)
- [Path: Window](../../path/window.md)
- [Module guide: se_window](../../module-guides/se-window.md)
- [Module guide: se_graphics](../../module-guides/se-graphics.md)
- [API: se_window.h](../../api-reference/modules/se_window.md)
