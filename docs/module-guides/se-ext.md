---
title: se_ext
summary: Runtime feature queries for instancing, MRT, float targets, and compute support.
prerequisites:
  - Public header include path available.
---

# se_ext

## When to use this

Use `se_ext` when an optimization or shader path depends on a runtime feature such as instancing, multiple render targets, float render targets, or compute support.

## Minimal working snippet

```c
#include "se_ext.h"

if (se_ext_is_supported(SE_EXT_FEATURE_INSTANCING)) {
	/* enable instanced path */
}
```

## Step-by-step explanation

1. Query support after backend initialization so optional code paths have a clear runtime gate.
1. Use `se_ext_require(...)` when the feature is mandatory and the caller should receive a normal engine error on failure.
1. Keep feature gates near the code that actually depends on them instead of scattering backend-specific assumptions across the project.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_ext.md).
1. Compare with the [Utilities path page](../path/utilities.md).

</div>

## Common mistakes

- Branching only on backend enums when the real requirement is a specific runtime feature.
- Enabling an advanced path before checking support on the active machine.
- Using feature probes as a substitute for broader backend or capability inspection when the decision is more complex.

## Related pages

- [Path: Utilities](../path/utilities.md)
- [Path: Backend](../path/backend.md)
- [Module guide: se_backend](se-backend.md)
- [Module guide: se_render_buffer](se-render-buffer.md)
- [Example: scene2d_instancing](../examples/default/scene2d_instancing.md)
