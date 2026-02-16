---
title: se_shader
summary: Shader load and uniform set APIs.
prerequisites:
  - Public header include path available.
---

# se_shader


## When to use this

Use `se_shader` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_shader.h"
se_shader_set_float(shader, "u_value", 1.0f);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown>

## Try this next

1. Open [API declarations](../api-reference/modules/se_shader.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Examples index](../examples/index.md)
- [API module index](../api-reference/modules/index.md)
- [Glossary terms](../glossary/terms.md)
