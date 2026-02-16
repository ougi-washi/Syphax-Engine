---
title: loader/se_loader
summary: General loader helper APIs.
prerequisites:
  - Public header include path available.
---

# loader/se_loader


## When to use this

Use `loader/se_loader` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "loader/se_loader.h"
// use loader helpers for asset reads
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown>

## Try this next

1. Open [API declarations](../api-reference/modules/loader_se_loader.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Examples index](../examples/index.md)
- [API module index](../api-reference/modules/index.md)
- [Glossary terms](../glossary/terms.md)
