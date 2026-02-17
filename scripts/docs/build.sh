#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MKDOCS_BIN="mkdocs"

if [[ -x "$ROOT_DIR/.venv/bin/mkdocs" ]]; then
	MKDOCS_BIN="$ROOT_DIR/.venv/bin/mkdocs"
elif ! command -v "$MKDOCS_BIN" >/dev/null 2>&1; then
	echo "mkdocs executable not found in PATH or .venv/bin/mkdocs" >&2
	exit 1
fi

cd "$ROOT_DIR"
rm -rf site
./scripts/docs/generate_api_reference.sh
./scripts/docs/check_nav_consistency.sh
./scripts/docs/check_links.sh
./scripts/docs/check_content_quality.sh
./scripts/docs/check_path_coverage.sh
./scripts/docs/verify_snippets.sh
"$MKDOCS_BIN" build --strict --config-file mkdocs.yml
./scripts/docs/check_path_render.sh
./scripts/docs/check_site_size.sh
