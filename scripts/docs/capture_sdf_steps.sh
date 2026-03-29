#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FRAME=5
TIMEOUT_SECONDS=25
SHOULD_BUILD=0

usage() {
	cat <<'EOF'
Usage: ./scripts/docs/capture_sdf_steps.sh [options]

Generate PNG captures for the four SDF path steps.

Options:
  --build                 Build the sdf target before capture.
  --frame <n>             Frame to capture (default: 5).
  --timeout <seconds>     Capture timeout per step (default: 25).
  --help                  Show this help.

Examples:
  ./scripts/docs/capture_sdf_steps.sh --build
  ./scripts/docs/capture_sdf_steps.sh --frame 30
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build)
			SHOULD_BUILD=1
			shift
			;;
		--frame)
			FRAME="${2:-}"
			shift 2
			;;
		--timeout)
			TIMEOUT_SECONDS="${2:-}"
			shift 2
			;;
		--help|-h)
			usage
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			usage
			exit 1
			;;
	esac
done

if ! [[ "$FRAME" =~ ^[0-9]+$ ]] || [[ "$FRAME" -lt 1 ]]; then
	echo "Invalid --frame value: $FRAME" >&2
	exit 1
fi
if ! [[ "$TIMEOUT_SECONDS" =~ ^[0-9]+$ ]] || [[ "$TIMEOUT_SECONDS" -lt 1 ]]; then
	echo "Invalid --timeout value: $TIMEOUT_SECONDS" >&2
	exit 1
fi

if [[ "$SHOULD_BUILD" -eq 1 ]]; then
	"$ROOT_DIR/build.sh" sdf_path_steps
fi

BINARY="$ROOT_DIR/bin/sdf_path_steps"
if [[ ! -x "$BINARY" ]]; then
	echo "Missing binary: $BINARY (build first or use --build)" >&2
	exit 1
fi

OUTPUT_DIR="$ROOT_DIR/docs/assets/img/path/sdf"
mkdir -p "$OUTPUT_DIR"

for step in 1 2 3 4; do
	output="$OUTPUT_DIR/step${step}.png"
	log_path="/tmp/se_docs_sdf_step_${step}.log"
	rm -f "$output"
	echo "Capturing SDF step $step -> ${output#$ROOT_DIR/}"

	if timeout "${TIMEOUT_SECONDS}s" env \
		SE_DOCS_SDF_STEP="$step" \
		SE_DOCS_CAPTURE_PATH="$output" \
		SE_DOCS_CAPTURE_FRAME="$FRAME" \
		SE_DOCS_CAPTURE_EXIT=1 \
		SE_TERMINAL_RENDER=1 \
		SE_TERMINAL_HIDE_WINDOW=1 \
		"$BINARY" >"$log_path" 2>&1; then
		:
	else
		status=$?
		echo "Capture failed for step $step (exit $status)" >&2
		tail -n 40 "$log_path" >&2 || true
		exit 1
	fi

	if [[ ! -s "$output" ]]; then
		echo "Capture output missing for step $step: $output" >&2
		tail -n 40 "$log_path" >&2 || true
		exit 1
	fi
done

echo "SDF step capture generation complete."
