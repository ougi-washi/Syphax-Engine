#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
API_DIR="$ROOT_DIR/docs/api-reference/modules"
SNAPSHOT_DIR="$(mktemp -d)"
cleanup() {
	rm -rf "$SNAPSHOT_DIR"
}
trap cleanup EXIT

cd "$ROOT_DIR"
cp -a "$API_DIR/." "$SNAPSHOT_DIR/"
./scripts/docs/generate_api_reference.sh
if ! diff -ru "$SNAPSHOT_DIR" "$API_DIR" >/tmp/se_api_sync_diff.log; then
	echo "Generated API docs are out of date. Run ./scripts/docs/generate_api_reference.sh and keep the updated files." >&2
	cat /tmp/se_api_sync_diff.log >&2
	exit 1
fi

echo "API docs are in sync with public headers."
