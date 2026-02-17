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
./scripts/docs/check_links.sh
./scripts/docs/check_content_quality.sh
./scripts/docs/check_path_coverage.sh
./scripts/docs/verify_snippets.sh
./scripts/docs/build.sh
```

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

## Add a new module Path

1. Copy `docs/templates/path-template.md` into `docs/path/<slug>.md`.
1. Create `docs/snippets/<module_id>/step1_minimal.c` through `step4_complete.c`.
1. Add links:
   - Section landing page -> Path
   - Module guide -> Path
   - Example page learning path -> Path step
   - Generated API page -> Path (via API generator mapping)
1. Run the required command set above before opening the PR.

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
- Module guides must link to deep-dive Path plus generated API pages.
- Path must link back to module guides, API pages, and at least one example page.

## Labels for docs issues

- `docs:bug`
- `docs:content`
