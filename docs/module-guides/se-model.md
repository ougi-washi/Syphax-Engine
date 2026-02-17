---
title: se_model
summary: Model loading and model handle APIs.
prerequisites:
  - Public header include path available.
---

# se_model


## When to use this

Use `se_model` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_model.h"
se_model_load_obj_simple(path, vs, fs);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_model.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive path page](../path/model.md)
- [API module page](../api-reference/modules/se_model.md)
- [Example: model_viewer](../examples/default/model_viewer.md)
