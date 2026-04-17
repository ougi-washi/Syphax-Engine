---
title: SDF
summary: Practical starting point for using Syphax SDFs without guessing which page to read first.
prerequisites:
  - Use links in this section.
---

# SDF

Use this page first if you want to render signed-distance content and do not know which of the other SDF pages matters yet.

## Start here

- If you want the smallest working setup, read [Path: SDF](../path/sdf.md) Step 1 and Step 2.
- If you want a complete small scene, run the [sdf example](../examples/default/sdf.md).
- If you want save/load or runtime editing, read [Module guide: se_sdf](../module-guides/se-sdf.md) and then [editor_sdf](../examples/advanced/editor_sdf.md).
- If you only want API declarations, jump to [API: se_sdf.h](../api-reference/modules/se_sdf.md).
- If you see `sdf_path_steps`, ignore it unless you are maintaining docs captures. It is a docs-helper target, not the normal user example.

## Smallest route

1. Create a context, window, and camera.
1. Create one `se_sdf` primitive.
1. Render it with `se_sdf_render_to_window(...)`.
1. Add children, noise, and lights only after the first primitive is visible.

## Choose the right page

- I want one visible SDF now:
  Read [Path: SDF](../path/sdf.md). It is the step-by-step learning page.
- I want the short explanation of the module:
  Read [Module guide: se_sdf](../module-guides/se-sdf.md). It explains what the module owns and which APIs matter most.
- I want a runnable reference target:
  Read [Example: sdf](../examples/default/sdf.md). It shows a small composed scene with shared lights and per-object shading.
- I want editing, JSON, or keyboard-driven tooling:
  Read [Example: editor_sdf](../examples/advanced/editor_sdf.md) and [Module guide: se_editor](../module-guides/se-editor.md).
- I want docs-maintenance capture code:
  Read [Example: sdf_path_steps](../examples/advanced/sdf_path_steps.md) only if you are updating the docs path screenshots.

## Common mistakes

- Starting with JSON editing or the editor example before rendering one primitive successfully.
- Attaching lights or noise without being clear whether they belong on the root SDF or a child.
- Treating every SDF page as part of the normal learning flow. For most users, the only pages they need are this page, `path/sdf`, and the `sdf` example.

## Related pages

- [Path: SDF](../path/sdf.md)
- [Module guide: se_sdf](../module-guides/se-sdf.md)
- [Module guide: se_noise](../module-guides/se-noise.md)
- [Example: sdf](../examples/default/sdf.md)
- [Example: editor_sdf](../examples/advanced/editor_sdf.md)
