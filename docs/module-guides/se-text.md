---
title: se_text
summary: Font loading and screen-space text drawing APIs.
prerequisites:
  - Public header include path available.
---

# se_text

## When to use this

Use `se_text` when a target needs straightforward font loading and text drawing outside the higher-level UI widget system.

## Minimal working snippet

```c
#include "se_text.h"

se_font_handle font = se_font_load(SE_RESOURCE_PUBLIC("fonts/ithaca.ttf"), 32.0f);
se_text_draw(font, "Label", &pos, &scale, 0.03f);
```

## Step-by-step explanation

1. Load a font once from a public or example resource path and keep the handle for repeated draws.
1. Draw text each frame from the owning window or overlay flow, using position, scale, and line spacing explicitly.
1. Keep text rendering separate from UI widgets unless the feature truly needs the UI event and layout system as well.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_text.md).
1. Continue with the [Text path page](../path/text.md).

</div>

## Common mistakes

- Reloading fonts every frame.
- Using the wrong resource scope for packaged font assets.
- Rebuilding a UI system when plain text draw calls are enough.

## Related pages

- [Path: Text](../path/text.md)
- [UI overview](../ui/index.md)
- [Module guide: se_ui](se-ui.md)
- [Example: hello_text](../examples/default/hello_text.md)
- [Example: ui_basics](../examples/default/ui_basics.md)
