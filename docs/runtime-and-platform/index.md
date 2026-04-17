---
title: Runtime And Platform
summary: Short route for windows, backend selection, queue-backed rendering, and runtime diagnostics.
prerequisites:
  - Use links in this section.
---

# Runtime And Platform

Use this page when you are setting up the app shell, validating a backend path, or debugging frame submission and runtime ownership.

## Start here

- Use [Start Here: Install and build](../start-here/install-and-build.md) for build flags and platform notes.
- Use [Path: Window](../path/window.md) for the canonical frame loop.
- Use [Path: Backend](../path/backend.md) when a problem depends on the active render/platform backend.

## Choose the right page

- I need the normal window loop:
  Read [Module guide: se_window](../module-guides/se-window.md).
- I need backend or capability info:
  Read [Module guide: se_backend](../module-guides/se-backend.md) and [Module guide: se_ext](../module-guides/se-ext.md).
- I need manual queue or render-thread diagnostics:
  Read [Module guide: se_render_frame](../module-guides/se-render-frame.md) and [Module guide: se_render_thread](../module-guides/se-render-thread.md).
- I need background CPU jobs:
  Read [Module guide: se_worker](../module-guides/se-worker.md).
- I need last-frame diagnostics or trace output:
  Read [Module guide: se_debug](../module-guides/se-debug.md).

## Useful runnable references

- [Example: context_lifecycle](../examples/advanced/context_lifecycle.md)
- [Example: debug_tools](../examples/advanced/debug_tools.md)
- [Example: rts_integration](../examples/advanced/rts_integration.md)

## Related pages

- [Path: Window](../path/window.md)
- [Path: Backend](../path/backend.md)
- [Debug and performance overview](../debug-and-performance/index.md)
- [Module guide: se_window](../module-guides/se-window.md)
- [Module guide: se_backend](../module-guides/se-backend.md)
