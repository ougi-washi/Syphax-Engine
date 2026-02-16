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
./scripts/docs/build.sh
```

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
- Example pages must include related API links.
- Module guides must link to generated API pages.

## Labels for docs issues

- `docs:bug`
- `docs:content`
