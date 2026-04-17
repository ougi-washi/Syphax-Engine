---
title: se_ui
summary: Widget tree, layout, callbacks, and frame-driven UI update/draw APIs.
prerequisites:
  - Public header include path available.
---

# se_ui

## When to use this

Use `se_ui` when a feature needs widgets, layout, focus, hover, click, text entry, or scroll behavior on top of a window.

## Minimal working snippet

```c
#include "se_ui.h"

se_ui_handle ui = se_ui_create(window, 64);
se_ui_tick(ui);
se_ui_draw(ui);
```

## Step-by-step explanation

1. Create one UI root per window, then add panels, text, buttons, textboxes, or scrollboxes under a clear widget hierarchy.
1. Register callbacks and layout/style data on widgets instead of pushing interaction logic down into loose scene objects.
1. Tick the UI before draw each frame so hover, focus, click, and text-entry state stay in sync with the current input snapshot.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_ui.md).
1. Continue with the [UI path page](../path/ui.md).

</div>

## Common mistakes

- Calling `se_ui_draw(...)` without calling `se_ui_tick(...)` first.
- Treating widgets as if they were generic scene objects instead of using the widget APIs that already manage state and callbacks.
- Rebuilding a full UI system for simple static labels that `se_text` could draw directly.

## Related pages

- [Path: UI](../path/ui.md)
- [UI overview](../ui/index.md)
- [Module guide: se_text](se-text.md)
- [Module guide: se_input](se-input.md)
- [Example: ui_basics](../examples/default/ui_basics.md)
- [Example: ui_showcase](../examples/advanced/ui_showcase.md)
