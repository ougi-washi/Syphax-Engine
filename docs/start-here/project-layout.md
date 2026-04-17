---
title: Project Layout
summary: Map repository folders to responsibilities.
prerequisites:
  - Repository cloned locally.
---

# Project Layout


## When to use this

Use when adding new docs pages, examples, or module changes.

## Minimal working snippet

```c
include/
src/
examples/
examples/advanced/
resources/
docs/
```

## Step-by-step explanation

1. Treat `include/` as public contract.
1. Treat `src/` as module implementation.
1. Treat `examples/` and `examples/advanced/` as runnable validation and teaching targets.
1. Treat `docs/` as authored docs source and `scripts/docs/` as docs maintenance tooling.
1. Keep generated output in `build/`, `bin/`, and `site/`.

<div class="next-block" markdown="1">

## Next

1. Next: [First window](first-window.md).
1. Cross-check with [Resource paths](resource-paths.md).

</div>

## Common mistakes

- Editing generated artifacts as if they were source.
- Using resource files from the wrong scope.

## Related pages

- [Resource paths](resource-paths.md)
- [Contributing docs](../contributing-docs.md)
- [API module index](../api-reference/modules/index.md)
- [Module guides](../module-guides/index.md)
- [Next path page](../path/window.md)
