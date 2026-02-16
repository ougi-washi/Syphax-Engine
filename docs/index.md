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
- [Path overview](path/index.md)
- [Examples index](examples/index.md)
- [Module guides](module-guides/index.md)
- [API reference](api-reference/index.md)
- [Glossary](glossary/terms.md)

## Documentation map

- `Start Here`: setup and first-run pages.
- `Path`: ordered pages that each explain one concept.
- `Examples`: one page per runnable target.
- `Module Guides`: usage-oriented pages mapped to API modules.
- `API Reference`: generated declaration pages from public headers.

## Build this docs site locally

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r docs/requirements.txt
./scripts/docs/build.sh
```
