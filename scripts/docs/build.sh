#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "$ROOT_DIR"
rm -rf site
./scripts/docs/generate_api_reference.sh
./scripts/docs/check_nav_consistency.sh
./scripts/docs/check_links.sh
./scripts/docs/check_content_quality.sh
mkdocs build --strict --config-file mkdocs.yml
./scripts/docs/check_site_size.sh
