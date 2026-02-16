---
title: Docs Build Troubleshooting
summary: Fix common docs build and link failures.
prerequisites:
  - Docs scripts available in scripts/docs/.
---

# Build And Links

## Install docs dependencies

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r docs/requirements.txt
```

## Validate nav and links

```bash
./scripts/docs/check_nav_consistency.sh
./scripts/docs/check_links.sh
```

## Rebuild full docs site

```bash
./scripts/docs/build.sh
```

## Common failures

- Missing nav entry for a new page.
- Broken relative path in markdown link.
- API docs stale after header updates.
