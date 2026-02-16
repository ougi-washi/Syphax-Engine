#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "$ROOT_DIR"
./scripts/docs/generate_api_reference.sh
if ! git diff --quiet -- docs/api-reference/modules; then
	echo "Generated API docs are out of date. Run ./scripts/docs/generate_api_reference.sh and commit changes." >&2
	git --no-pager diff -- docs/api-reference/modules
	exit 1
fi

echo "API docs are in sync with public headers."
