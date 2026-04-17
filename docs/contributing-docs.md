---
title: Contributing Docs
summary: Process and quality gates for docs updates.
prerequisites:
  - Read Start Here and Path overviews.
---

# Contributing Docs

## Ownership and review

- Branch naming: `docs/<scope>-<topic>`
- Merge policy: PR review plus required checks
- Required checks: docs generation, nav consistency, link check, strict build

## Required commands

```bash
./scripts/docs/generate_api_reference.sh
./scripts/docs/check_nav_consistency.sh
./scripts/docs/check_example_coverage.sh
./scripts/docs/check_links.sh
./scripts/docs/check_content_quality.sh
./scripts/docs/check_path_coverage.sh
./scripts/docs/verify_snippets.sh
./scripts/docs/build.sh
```

## Coverage model

- `Path` is curated. Not every public header needs a deep-dive path page.
- `Module Guides` are broader. Every public header that appears in generated API reference should also have an authored module guide page and nav entry.
- `Examples` should stay one page per runnable target.

## Path authoring workflow

1. Create or update one page under `docs/path/`.
1. Add exactly four snippet files under `docs/snippets/<module_id>/`:
   - `step1_minimal.c`
   - `step2_core.c`
   - `step3_interactive.c`
   - `step4_complete.c`
1. Keep required Path headings:
   - `## When to use this`
   - `## Step 1: Minimal Working Project`
   - `## Step 2: Add Core Feature`
   - `## Step 3: Interactive / Tunable`
   - `## Step 4: Complete Practical Demo`
   - `## Common mistakes`
   - `## Next`
   - `## Related pages`
1. Add at least one link to an API module page and one link to an example page.
1. Add or update the page in `mkdocs.yml` navigation.

## Module guide workflow

1. Create or update one page under `docs/module-guides/`.
1. Keep the required concept-page headings:
   - `## When to use this`
   - `## Minimal working snippet`
   - `## Step-by-step explanation`
   - `## Next`
   - `## Common mistakes`
   - `## Related pages`
1. Link the guide to the generated API page for the same public header.
1. Link the guide to the best matching Path page when one exists; otherwise link to the most relevant topic landing page or example page.
1. Add the guide to both `docs/module-guides/index.md` and the `Module Guides` section of `mkdocs.yml`.

## Add a new module Path

1. Copy `docs/templates/path-template.md` into `docs/path/<slug>.md`.
1. Create `docs/snippets/<module_id>/step1_minimal.c` through `step4_complete.c`.
1. Add links:
   - Section landing page -> Path
   - Module guide -> Path
   - Example page learning path -> Path step
   - Generated API page -> Path (via API generator mapping)
1. Run the required command set above before opening the PR.

## Add or rename an example target

1. Create or update the reference page under `docs/examples/default/` or `docs/examples/advanced/`.
1. Add the page to both `docs/examples/index.md` and the `Examples` nav section in `mkdocs.yml`.
1. Add a placeholder image under `docs/assets/img/examples/<track>/<target>.svg`.
1. Update `scripts/docs/capture_examples.sh` if the target should participate in local screenshot capture.
1. Run `./scripts/docs/check_example_coverage.sh` before opening the PR.

## Add or rename a public module

1. Update the public header under `include/` or `include/loader/`.
1. Regenerate API reference pages with `./scripts/docs/generate_api_reference.sh`.
1. Add or update the matching authored module guide page.
1. Add the guide to `docs/module-guides/index.md` and `mkdocs.yml`.
1. Add or update related links from landing pages when the module changes the visible public surface.
1. Run the full command set above before opening the PR.

## Release cadence

- Update docs whenever public headers, examples, or controls change.
- Run monthly docs drift checks in CI.

## Support matrix

- Current stable Chrome, Firefox, Safari
- Desktop and mobile layouts

## Accessibility baseline

- Semantic heading order
- Keyboard navigability
- Visible focus states
- Sufficient contrast

## Metadata rules

Use front matter on each page:

- `title`
- `summary`
- `prerequisites`

## Cross-linking rules

- Concept pages must include a `Related pages` section.
- Example pages must include related API links and a learning-path mapping to Path steps.
- Module guides must link to generated API pages and to the clearest matching Path page, topic landing page, or example page.
- Path must link back to module guides, API pages, and at least one example page.

## Labels for docs issues

- `docs:bug`
- `docs:content`
