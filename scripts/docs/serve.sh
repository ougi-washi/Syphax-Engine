#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MKDOCS_BIN="mkdocs"
PYTHONPATH_PREFIX="$ROOT_DIR/scripts/docs/python"

if [[ -x "$ROOT_DIR/.venv/bin/mkdocs" ]]; then
	MKDOCS_BIN="$ROOT_DIR/.venv/bin/mkdocs"
elif ! command -v "$MKDOCS_BIN" >/dev/null 2>&1; then
	echo "mkdocs executable not found in PATH or .venv/bin/mkdocs" >&2
	exit 1
fi

cd "$ROOT_DIR"
./scripts/docs/generate_api_reference.sh
PYTHONPATH="$PYTHONPATH_PREFIX${PYTHONPATH:+:$PYTHONPATH}" "$MKDOCS_BIN" serve --config-file mkdocs.yml
