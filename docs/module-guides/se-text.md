---
title: se_text
summary: Text and font rendering APIs.
prerequisites:
  - Public header include path available.
---

# se_text


## When to use this

Use `se_text` for features that map directly to its module boundary.

## Minimal working snippet

```c
#include "se_text.h"
se_font_handle font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
se_text_draw(font, "Label", &pos, &scale, 0.03f);
```

## Step-by-step explanation

1. Include the module header and load a font once.
1. Call per-frame functions in your frame loop where needed.
1. Destroy resources before context teardown (or rely on full context teardown).

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_text.md).
1. Run one linked example and change one parameter.

</div>

## Common mistakes

- Skipping creation failure checks.
- Using this module to encode domain-specific framework logic in engine core.

## Related pages

- [Deep dive path page](../path/text.md)
- [API module page](../api-reference/modules/se_text.md)
- [Example: hello_text](../examples/default/hello_text.md)
