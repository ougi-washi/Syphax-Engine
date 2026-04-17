---
title: se_defines
summary: Result codes, resource-path macros, and platform-aware path resolution helpers.
prerequisites:
  - Public header include path available.
---

# se_defines

## When to use this

Use `se_defines` when a feature needs shared result codes, packaged resource path macros, or writable path resolution that works across host and packaged runtimes.

## Minimal working snippet

```c
#include "se_defines.h"

const char* message = se_result_str(se_get_last_error());
se_paths_resolve_writable_path(buffer, sizeof(buffer), "state/save.json");
```

## Step-by-step explanation

1. Use the `SE_RESOURCE_*` macros to keep packaged asset scope explicit in every load call.
1. Read and set result codes through the shared error helpers so failures stay consistent across modules.
1. Resolve writable paths before saving files so host and packaged runtimes do not diverge on where output should go.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_defines.md).
1. Compare with the [Utilities path page](../path/utilities.md).

</div>

## Common mistakes

- Concatenating resource roots manually instead of using the provided resource macros.
- Writing save files into packaged resource paths instead of resolving a writable path first.
- Swallowing `se_get_last_error()` state when a caller actually needs the module failure reason.

## Related pages

- [Path: Utilities](../path/utilities.md)
- [Start Here: Resource paths](../start-here/resource-paths.md)
- [Start Here: Error handling](../start-here/error-handling.md)
- [Module guide: se_ext](se-ext.md)
- [Example: hello_text](../examples/default/hello_text.md)
