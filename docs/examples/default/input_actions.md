---
title: input_actions
summary: Walkthrough page for input_actions.
prerequisites:
  - Build toolchain and resources available.
---

# input_actions

<picture>
  <source srcset="../../../assets/img/examples/default/input_actions.png" type="image/png">
  <img src="../../../assets/img/examples/default/input_actions.svg" alt="input_actions preview image">
</picture>

*Caption: live runtime capture if available; falls back to placeholder preview card.*

## Goal

Bind keyboard and mouse inputs to actions and move a 2D actor.


## Learning path

- This example corresponds to [Input path page](../../path/input.md) Step 3.
- Next: apply one change from the linked path step and rerun this target.
## Controls

- W A S D: move square
- Left Shift: boost speed
- Left click: toggle color
- Esc: quit

## Build command

```bash
./build.sh input_actions
```

## Run command

```bash
./bin/input_actions
```

## Edits to try

1. Remap movement to arrow keys.
1. Adjust boost multiplier.
1. Add a reset-position action.

## Related API links

- [Path: Input](../../path/input.md)
- [Module guide: se_input](../../module-guides/se-input.md)
- [API: se_input.h](../../api-reference/modules/se_input.md)
