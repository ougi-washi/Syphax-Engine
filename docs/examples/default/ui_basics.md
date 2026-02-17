---
title: ui_basics
summary: Reference for ui_basics example.
prerequisites:
  - Build toolchain and resources available.
---

# ui_basics

<img src="../../../assets/img/examples/default/ui_basics.png" alt="ui_basics preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/ui_basics.svg';">


## Goal

Build a small UI with two buttons and text updates from callbacks.


## Learning path

- This example corresponds to [UI path page](../../path/ui.md) Step 2.

## Controls

- Click buttons: update/reset counter
- D: toggle debug overlay
- Esc: quit

## Build command

```bash
./build.sh ui_basics
```

## Run command

```bash
./bin/ui_basics
```

## Internal flow

- The UI tree is built once from a root widget with vertical stack layout.
- Button callbacks mutate shared state and push new label text through `se_ui_widget_set_text`.
- `se_ui_tick` resolves input/focus/click events, then `se_ui_draw` emits the widget pass.

## Related API links

- [Source code: `examples/ui_basics.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/ui_basics.c)
- [Path: UI](../../path/ui.md)
- [Module guide: se_ui](../../module-guides/se-ui.md)
- [API: se_ui.h](../../api-reference/modules/se_ui.md)
