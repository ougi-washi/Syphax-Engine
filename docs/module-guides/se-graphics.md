---
title: se_graphics
summary: Render clear and graphics state helper APIs.
prerequisites:
  - Public header include path available.
---

# se_graphics


## When to use this

Use `se_graphics` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_graphics.h"
se_render_clear();
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_graphics.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive Playbook](../playbooks/se-graphics.md)
- [API module page](../api-reference/modules/se_graphics.md)
- [Example: hello_text](../examples/default/hello_text.md)
- [Example: scene2d_click](../examples/default/scene2d_click.md)
