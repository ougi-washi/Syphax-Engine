---
title: multi_window
summary: Walkthrough page for multi_window.
prerequisites:
  - Build toolchain and resources available.
---

# multi_window

> Scope: advanced

![multi_window preview](../../assets/img/examples/advanced/multi_window.svg)

*Caption: representative preview panel for `multi_window`.*

## Goal

Render three windows in one context with per-window clear colors.

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

## Edits to try

1. Change each window color.
1. Set target FPS values per window.
1. Add per-window key toggles.

## Related API links

- [Module guide: se_window](../../module-guides/se-window.md)
- [Module guide: se_graphics](../../module-guides/se-graphics.md)
- [API: se_window.h](../../api-reference/modules/se_window.md)
