---
title: Home
summary: Main entry for Syphax Engine documentation.
prerequisites:
  - None
---

# Syphax Engine Documentation

<div class="hero-card" markdown="1">

Syphax Engine is a C framework for interactive visuals, games, and tools.

<div class="hero-grid">
<div class="hero-pill">Start with setup and first run</div>
<div class="hero-pill">Follow one concept per page</div>
<div class="hero-pill">Use example walkthrough pages</div>
<div class="hero-pill">Jump to module guides and API pages</div>
</div>

</div>

## Quick links

- [Install and build](start-here/install-and-build.md)
- [First window](start-here/first-window.md)
- [Runtime and platform](runtime-and-platform/index.md)
- [Loading and assets](loading-and-assets/index.md)
- [Utilities](utilities/index.md)
- [SDF](sdf/index.md)
- [Path overview](path/index.md)
- [Examples index](examples/index.md)
- [Module guides](module-guides/index.md)
- [API module index](api-reference/modules/index.md)
- [API reference](api-reference/index.md)
- [Glossary](glossary/terms.md)

## Documentation map

- `Start Here`: setup and first-run pages.
- `Path`: a curated learning route with one concept per page and four-step snippets.
- `Examples`: one page per runnable target.
- `Module Guides`: short usage guides that map the full public module surface to real tasks.
- `API Reference`: generated declaration pages from public headers.

## Use this site effectively

- Start in `Start Here` if this is a fresh checkout or a first build.
- Use `Task Guides` when you want the shortest answer to "which page do I read for this kind of task?"
- Use `Path` when learning one subsystem from minimal setup to a fuller demo.
- Use `Module Guides` when you already know the module name and want the shortest route to related docs.
- Use `Examples` when you want a runnable target to inspect or extend.

## Build this docs site locally

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r docs/requirements.txt
./scripts/docs/build.sh
```
