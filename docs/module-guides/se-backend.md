---
title: se_backend
summary: Active backend, shader profile, portability, and capability queries.
prerequisites:
  - Public header include path available.
---

# se_backend

## When to use this

Use `se_backend` when feature availability depends on the active render backend, platform backend, shader profile, or capability limits.

## Minimal working snippet

```c
#include "se_backend.h"

se_backend_info info = se_get_backend_info();
se_capabilities caps = se_capabilities_get();
```

## Step-by-step explanation

1. Query backend and capability state early in startup so optional features and shader paths can be selected deliberately.
1. Combine backend info with `se_ext` checks when an optimization depends on both platform intent and runtime support.
1. Use backend data when debugging portability issues, queue behavior, or render limits instead of hard-coding assumptions by platform name.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_backend.md).
1. Continue with the [Backend path page](../path/backend.md).

</div>

## Common mistakes

- Treating planned backends as if they were implemented runtime targets.
- Selecting advanced rendering paths without checking actual capability data.
- Using backend enums alone when the real gate is an extension or capability query.

## Related pages

- [Path: Backend](../path/backend.md)
- [Module guide: se_ext](se-ext.md)
- [Module guide: se_render_frame](se-render-frame.md)
- [Module guide: se_render_thread](se-render-thread.md)
- [Example: context_lifecycle](../examples/advanced/context_lifecycle.md)
