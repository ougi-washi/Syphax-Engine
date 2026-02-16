---
title: se_backend
summary: Backend descriptor and runtime backend APIs.
prerequisites:
  - Public header include path available.
---

# se_backend


## When to use this

Use `se_backend` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_backend.h"
se_backend_get_runtime_info(&info);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_backend.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive Playbook](../playbooks/se-backend.md)
- [API module page](../api-reference/modules/se_backend.md)
- [Example: context_lifecycle](../examples/advanced/context_lifecycle.md)
