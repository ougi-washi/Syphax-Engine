---
title: se_vfx
summary: Emitter and particle APIs for 2D and 3D.
prerequisites:
  - Public header include path available.
---

# se_vfx

<div class="learn-block" markdown>

## What you will learn

- What `se_vfx` is responsible for.
- The smallest setup sequence for this module.
- How to jump from guide to generated API declarations.

</div>

## When to use this

Use `se_vfx` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_vfx.h"
se_vfx_2d_add_emitter(vfx, &params);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown>

## Try this next

1. Open [API declarations](../api-reference/modules/se_vfx.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Examples index](../examples/index.md)
- [API module index](../api-reference/modules/index.md)
- [Glossary terms](../glossary/terms.md)
