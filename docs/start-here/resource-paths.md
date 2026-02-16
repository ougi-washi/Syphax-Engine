---
title: Resource Paths
summary: Use internal/public/example path macros correctly.
prerequisites:
  - Project layout familiarity.
---

# Resource Paths

<div class="learn-block" markdown>

## What you will learn

- Resource scope intent.
- Macro-based path construction.
- How scopes affect portability and ownership.

</div>

## When to use this

Use for every asset path in examples and app code.

## Minimal working snippet

```c
SE_RESOURCE_INTERNAL("shaders/render_quad_frag.glsl");
SE_RESOURCE_PUBLIC("fonts/ithaca.ttf");
SE_RESOURCE_EXAMPLE("scene3d/scene3d_vertex.glsl");
```

## Step-by-step explanation

1. Pick the scope that matches ownership.
1. Keep paths relative inside each scope.
1. Avoid absolute host paths.

<div class="next-block" markdown>

## Try this next

1. Next page: [First run checklist](first-run-checklist.md).
1. Apply scope checks in [load a model](../path/load-a-model.md).

</div>

## Common mistakes

- Using internal assets in app-facing examples.
- Hardcoding host-specific absolute paths.

## Related pages

- [Project layout](project-layout.md)
- [Path: load a model](../path/load-a-model.md)
- [API: se_defines.h](../api-reference/modules/se_defines.md)
