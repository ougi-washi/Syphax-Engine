---
title: ui_basics
summary: Walkthrough page for ui_basics.
prerequisites:
  - Build toolchain and resources available.
---

# ui_basics

<img src="../../../assets/img/examples/default/ui_basics.png" alt="ui_basics preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/ui_basics.svg';">

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Build a small UI with two buttons and text updates from callbacks.


## Learning path

- This example corresponds to [UI path page](../../path/ui.md) Step 2.
- Next: apply one change from the linked path step and rerun this target.
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

## Edits to try

1. Add a new button action.
1. Adjust layout spacing.
1. Disable a widget and display state text.

## Related API links

- [Path: UI](../../path/ui.md)
- [Module guide: se_ui](../../module-guides/se-ui.md)
- [API: se_ui.h](../../api-reference/modules/se_ui.md)
