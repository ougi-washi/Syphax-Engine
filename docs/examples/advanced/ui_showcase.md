---
title: ui_showcase
summary: Reference for ui_showcase example.
prerequisites:
  - Build toolchain and resources available.
---

# ui_showcase

> Scope: advanced

<img src="../../../assets/img/examples/advanced/ui_showcase.png" alt="ui_showcase preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/advanced/ui_showcase.svg';">


## Goal

Advanced UI interactions with dynamic layout and widget states.


## Learning path

- This example corresponds to [UI path page](../../path/ui.md) Step 3.

## Controls

- Use startup control list printed by target
- Esc: quit

## Build command

```bash
./build.sh ui_showcase
```

## Run command

```bash
./bin/ui_showcase
```

## Internal flow

- A larger widget graph is built with layout containers, docking, clipping, and dynamic lists.
- Hover/focus/press/scroll callbacks feed live state labels and interaction counters.
- Runtime toggles exercise widget flags and layout recomputation under real input traffic.

## Related API links

- [Path: UI](../../path/ui.md)
- [Module guide: se_ui](../../module-guides/se-ui.md)
- [Default example: ui_basics](../default/ui_basics.md)
- [API: se_ui.h](../../api-reference/modules/se_ui.md)
