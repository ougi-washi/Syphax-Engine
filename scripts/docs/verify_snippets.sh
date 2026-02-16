#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SNIPPETS_DIR="$ROOT_DIR/docs/snippets"
CC_BIN="${CC:-cc}"

if [[ ! -d "$SNIPPETS_DIR" ]]; then
	echo "Missing snippets directory: $SNIPPETS_DIR" >&2
	exit 1
fi

mapfile -t snippet_files < <(find "$SNIPPETS_DIR" -type f -name '*.c' | sort)
if [[ ${#snippet_files[@]} -eq 0 ]]; then
	echo "No snippet sources found under $SNIPPETS_DIR" >&2
	exit 1
fi

for snippet in "${snippet_files[@]}"; do
	rel="${snippet#$ROOT_DIR/}"
	if ! out="$("$CC_BIN" -std=c11 -Wall -Wextra -fsyntax-only \
		-I"$ROOT_DIR/include" \
		-I"$ROOT_DIR/lib" \
		-I"$ROOT_DIR/lib/stb" \
		"$snippet" 2>&1)"; then
		echo "Snippet compile failed: $rel" >&2
		echo "$out" >&2
		exit 1
	fi
done

echo "Snippet verification passed (${#snippet_files[@]} snippets compile)."
