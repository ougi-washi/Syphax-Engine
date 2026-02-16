---
title: hello_text
summary: Walkthrough page for hello_text.
prerequisites:
  - Build toolchain and resources available.
---

# hello_text

![hello_text preview](../../assets/img/examples/default/hello_text.svg)

*Caption: representative preview panel for `hello_text`.*

## Goal

Create a window, render text, and toggle the background palette.

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

## Edits to try

1. Change the text string.
1. Increase font size from 34.0f to 48.0f.
1. Add a third color state.

## Related API links

- [Path: draw your first text](../../path/draw-your-first-text.md)
- [Module guide: se_text](../../module-guides/se-text.md)
- [API: se_window.h](../../api-reference/modules/se_window.md)
