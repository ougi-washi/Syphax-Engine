---
title: se_editor
summary: Generic editor data model, shortcuts, typed values, and command dispatch.
prerequisites:
  - Public header include path available.
---

# se_editor

## When to use this

Use `se_editor` when you need a generic, engine-level editor data model with items, typed properties, commands, and keyboard shortcut handling.

## Minimal working snippet

```c
#include "se_editor.h"

se_editor* editor = se_editor_create(&config);
se_editor_bind_default_shortcuts(editor);
```

## Step-by-step explanation

1. Provide callbacks that collect items, collect properties for the selected item, and apply commands back into your runtime data.
1. Bind shortcuts and poll shortcut events each frame so editor mode changes and command actions stay separate from rendering code.
1. Treat `se_editor` as a reusable data-and-command layer; your app or example still decides how to render the editor UI and how to persist changes.

<div class="next-block" markdown="1">

## Next

1. Open [API declarations](../api-reference/modules/se_editor.md).
1. Inspect the [editor_sdf example](../examples/advanced/editor_sdf.md).

</div>

## Common mistakes

- Assuming `se_editor` renders a full editor UI by itself.
- Using unstable labels or item selectors so commands target different runtime objects across refreshes.
- Mixing shortcut collection, command application, and custom UI rendering into one opaque callback.

## Related pages

- [Interaction overview](../interaction/index.md)
- [UI overview](../ui/index.md)
- [Path: Utilities](../path/utilities.md)
- [Example: editor_sdf](../examples/advanced/editor_sdf.md)
- [Module guide: se_ui](se-ui.md)
