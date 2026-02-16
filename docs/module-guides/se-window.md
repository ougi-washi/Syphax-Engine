---
title: se_window
summary: Window lifecycle and per-frame APIs.
prerequisites:
  - Public header include path available.
---

# se_window


## When to use this

Use `se_window` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_window.h"
se_window_begin_frame(window);
se_window_end_frame(window);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown>

## Try this next

1. Open [API declarations](../api-reference/modules/se_window.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Examples index](../examples/index.md)
- [API module index](../api-reference/modules/index.md)
- [Glossary terms](../glossary/terms.md)
