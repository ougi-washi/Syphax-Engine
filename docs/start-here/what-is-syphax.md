---
title: What Is Syphax
summary: Scope and architectural intent of the framework.
prerequisites:
  - A C11 compiler.
  - Basic repository navigation.
---

# What Is Syphax

<div class="learn-block" markdown>

## What you will learn

- What the framework includes.
- What remains app-side by design.
- How module APIs are organized.

</div>

## When to use this

Read first, before implementing app-level architecture on top of the engine.

## Minimal working snippet

```c
#include "se_window.h"
#include "se_scene.h"
#include "se_ui.h"

// choose only modules needed by your feature
```

## Step-by-step explanation

1. Create one context at startup.
1. Pull in only required modules.
1. Keep domain logic outside engine core.

<div class="next-block" markdown>

## Try this next

1. Next page: [Install and build](install-and-build.md).
1. Open [Module guides](../module-guides/index.md) after first run.

</div>

## Common mistakes

- Treating framework modules as a built-in gameplay framework.
- Mixing internal and public assets without scope checks.

## Related pages

- [Project layout](project-layout.md)
- [Glossary terms](../glossary/terms.md)
- [API reference index](../api-reference/index.md)
