---
title: input_actions
summary: Reference for input_actions example.
prerequisites:
  - Build toolchain and resources available.
---

# input_actions

<img src="../../../assets/img/examples/default/input_actions.png" alt="input_actions preview image" onerror="this.onerror=null;this.src='../../../assets/img/examples/default/input_actions.svg';">


## Goal

Bind keyboard and mouse inputs to actions and move a 2D actor.


## Learning path

- This example corresponds to [Input path page](../../path/input.md) Step 3.

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

## Internal flow

- `se_input_create` builds an action map over raw keyboard/mouse sources for one input context.
- `se_input_tick` resolves pressed/down/value states, then movement integrates with `delta_time`.
- Action-triggered state changes update scene data (`se_object_2d_set_position`, shader uniforms) before draw.

## Related API links

- [Source code: `examples/input_actions.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/input_actions.c)
- [Path: Input](../../path/input.md)
- [Module guide: se_input](../../module-guides/se-input.md)
- [API: se_input.h](../../api-reference/modules/se_input.md)
