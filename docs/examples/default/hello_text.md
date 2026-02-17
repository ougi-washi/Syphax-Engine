---
title: hello_text
summary: Reference for hello_text example.
prerequisites:
  - Build toolchain and resources available.
---

# hello_text

<img src="../../../assets/img/examples/default/hello_text.png" alt="hello_text preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/hello_text.svg';">


## Goal

Create a window, render text, and toggle the background palette.


## Learning path

- This example corresponds to [Window path page](../../path/window.md) Step 1.
- This example corresponds to [Text path page](../../path/text.md) Step 2.

## Controls

- Esc: quit
- Space: switch background color

## Build command

```bash
./build.sh hello_text
```

## Run command

```bash
./bin/hello_text
```

## Internal flow

- `se_context_create` + `se_window_create` set up the runtime and the default frame loop state.
- `se_window_is_key_pressed(SE_KEY_SPACE)` toggles the active palette branch before clear.
- `se_font_load` initializes one reusable font handle, and `se_text_draw` queues glyph draws each frame before `se_window_end_frame` submits.

## Related API links

- [Source code: `examples/hello_text.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/hello_text.c)
- [Path: Window](../../path/window.md)
- [Path: Text](../../path/text.md)
- [Module guide: se_text](../../module-guides/se-text.md)
- [API: se_window.h](../../api-reference/modules/se_window.md)
