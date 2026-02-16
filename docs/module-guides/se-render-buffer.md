---
title: se_render_buffer
summary: Offscreen render buffer APIs.
prerequisites:
  - Public header include path available.
---

# se_render_buffer


## When to use this

Use `se_render_buffer` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_render_buffer.h"
se_render_buffer_create(640, 360, shader_path);
```

## Step-by-step explanation

1. Include the module header and create required handles.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_render_buffer.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive Playbook](../playbooks/se-render-buffer.md)
- [API module page](../api-reference/modules/se_render_buffer.md)
- [Example: multi_window](../examples/advanced/multi_window.md)
