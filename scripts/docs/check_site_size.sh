#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SITE_DIR="$ROOT_DIR/site"
MAX_SITE_MB="${MAX_SITE_MB:-80}"

if [[ ! -d "$SITE_DIR" ]]; then
	echo "Site directory does not exist: $SITE_DIR" >&2
	exit 1
fi

site_bytes=$(du -sb "$SITE_DIR" | awk '{print $1}')
limit_bytes=$((MAX_SITE_MB * 1024 * 1024))

printf 'Built site size: %s bytes (limit: %s bytes)\n' "$site_bytes" "$limit_bytes"

if (( site_bytes > limit_bytes )); then
	echo "Site size exceeds budget (${MAX_SITE_MB} MB)." >&2
	exit 1
fi

oversized=$(find "$ROOT_DIR/docs/assets" -type f \( -name '*.png' -o -name '*.jpg' -o -name '*.jpeg' -o -name '*.webp' \) -size +600k -print || true)
if [[ -n "$oversized" ]]; then
	echo "Oversized assets detected (over 600KB each):" >&2
	echo "$oversized" >&2
	exit 1
fi

echo "Site size and static asset budget checks passed."
