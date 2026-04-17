---
title: se_navigation
summary: Grid and field pathfinding, reachability, and world/cell conversion APIs.
prerequisites:
  - Public header include path available.
---

# se_navigation

## When to use this

Use `se_navigation` when an app needs grid-based paths, sampled navigation fields, reachable-area queries, or world/cell conversion helpers.

## Minimal working snippet

```c
#include "se_navigation.h"

se_navigation_find_path_simple(&grid, start, goal, true, &path);
```

## Step-by-step explanation

1. Create a grid or field that matches the world representation you want to navigate.
1. Mark blocked cells or sampled costs explicitly so path queries see the same walkability rules as the rest of the app.
1. Reuse `se_navigation_path` and related containers across queries to keep the navigation layer predictable and cheap to reason about.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_navigation.md).
1. Continue with the [Navigation path page](../path/navigation.md).

</div>

## Common mistakes

- Forgetting to initialize or reset path/area containers before reuse.
- Mixing grid cells and world coordinates without a single conversion boundary.
- Treating debug-only navigation traces as the authoritative path output.

## Related pages

- [Path: Navigation](../path/navigation.md)
- [Physics overview](../physics/index.md)
- [Module guide: se_math](se-math.md)
- [Example: navigation_grid](../examples/advanced/navigation_grid.md)
