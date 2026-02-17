---
title: navigation_grid
summary: Reference for navigation_grid example.
prerequisites:
  - Build toolchain and resources available.
---

# navigation_grid

> Scope: advanced

<img src="../../../assets/img/examples/advanced/navigation_grid.svg" alt="navigation_grid preview image">


## Goal

Run pathfinding and raycast queries over a blocked grid.


## Learning path

- This example corresponds to [Navigation path page](../../path/navigation.md) Step 3.

## Controls

- Non-interactive example. Inspect stdout diagnostics.

## Build command

```bash
./build.sh navigation_grid
```

## Run command

```bash
./bin/navigation_grid
```

## Internal flow

- A navigation grid is initialized, then static/dynamic blockers are stamped into occupancy.
- Path search runs from start to goal, followed by path smoothing on the same grid snapshot.
- A raycast query reports line-of-sight traversal to compare against pathfinding output.

## Related API links

- [Source code: `examples/advanced/navigation_grid.c`](https://github.com/ougi-washi/Syphax-Engine/blob/main/examples/advanced/navigation_grid.c)
- [Path: Navigation](../../path/navigation.md)
- [Module guide: se_navigation](../../module-guides/se-navigation.md)
- [Glossary: raycast](../../glossary/terms.md#raycast)
- [API: se_navigation.h](../../api-reference/modules/se_navigation.md)
