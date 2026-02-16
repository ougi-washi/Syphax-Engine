---
title: Project Layout
summary: Map repository folders to responsibilities.
prerequisites:
  - Repository cloned locally.
---

# Project Layout

<div class="learn-block" markdown>

## What you will learn

- Where public headers live.
- Where modules are implemented.
- Where example assets live.

</div>

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
1. Keep generated output in `build/` and `bin/`.

<div class="next-block" markdown>

## Try this next

1. Next page: [First window](first-window.md).
1. Cross-check with [Resource paths](resource-paths.md).

</div>

## Common mistakes

- Editing generated artifacts as if they were source.
- Using resource files from the wrong scope.

## Related pages

- [Resource paths](resource-paths.md)
- [Contributing docs](../contributing-docs.md)
- [API module index](../api-reference/modules/index.md)
