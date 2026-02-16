#!/usr/bin/env bash
set -euo pipefail

EXAMPLES_DIR="examples"
ALLOWLIST_FILE="scripts/example_line_budget_allowlist.txt"
MAX_DEFAULT=150
MAX_EXCEPTION=180
MAX_EXCEPTION_COUNT=2

if ! command -v find >/dev/null 2>&1; then
	echo "line-budget :: missing 'find' command"
	exit 1
fi

declare -A allow_max
if [[ -f "$ALLOWLIST_FILE" ]]; then
	while IFS='|' read -r file max reason; do
		[[ -z "${file// }" ]] && continue
		[[ "$file" =~ ^# ]] && continue
		if [[ -z "$max" || ! "$max" =~ ^[0-9]+$ ]]; then
			echo "line-budget :: invalid allowlist entry for '$file'"
			exit 1
		fi
		allow_max["$file"]="$max"
	done < "$ALLOWLIST_FILE"
fi

mapfile -t beginner_files < <(find "$EXAMPLES_DIR" -maxdepth 1 -type f -name '*.c' | sort)
if [[ ${#beginner_files[@]} -eq 0 ]]; then
	echo "line-budget :: no beginner examples found in $EXAMPLES_DIR"
	exit 1
fi

printf "line-budget :: beginner files (default <=%d, exception <=%d)\n" "$MAX_DEFAULT" "$MAX_EXCEPTION"
printf "%-32s %8s %8s %s\n" "file" "lines" "limit" "status"

exception_count=0
failures=0
for path in "${beginner_files[@]}"; do
	file="$(basename "$path")"
	lines="$(wc -l < "$path" | tr -d ' ')"
	limit="$MAX_DEFAULT"
	if [[ -n "${allow_max[$file]:-}" ]]; then
		limit="${allow_max[$file]}"
	fi

	if (( limit > MAX_DEFAULT )); then
		exception_count=$((exception_count + 1))
	fi

	status="ok"
	if (( limit > MAX_EXCEPTION )); then
		status="fail(limit>${MAX_EXCEPTION})"
		failures=$((failures + 1))
	elif (( lines > limit )); then
		status="fail(over by $((lines - limit)))"
		failures=$((failures + 1))
	fi

	printf "%-32s %8d %8d %s\n" "$file" "$lines" "$limit" "$status"
done

if (( exception_count > MAX_EXCEPTION_COUNT )); then
	echo "line-budget :: too many exceptions (${exception_count}), max is ${MAX_EXCEPTION_COUNT}"
	exit 1
fi

if (( failures > 0 )); then
	echo "line-budget :: failed with ${failures} issue(s)"
	exit 1
fi

echo "line-budget :: passed"
