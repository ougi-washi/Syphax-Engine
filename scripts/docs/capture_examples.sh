#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FRAME=20
TIMEOUT_SECONDS=20
SHOULD_BUILD=0
USE_TERMINAL=1
TRACK_FILTER="all"

declare -a DEFAULT_TARGETS=(
	hello_text
	input_actions
	scene2d_click
	scene2d_instancing
	scene3d_orbit
	physics2d_playground
	physics3d_playground
	ui_basics
	audio_basics
	model_viewer
)

declare -a ADVANCED_TARGETS=(
	array_handles
	context_lifecycle
	debug_tools
	gltf_roundtrip
	multi_window
	navigation_grid
	rts_integration
	sdf_playground
	simulation_intro
	simulation_advanced
	ui_showcase
	vfx_emitters
)

declare -A SKIP_REASON_BY_TARGET=(
	[array_handles]="non-visual stdout reference example"
	[context_lifecycle]="non-visual lifecycle/reference example"
	[gltf_roundtrip]="non-visual conversion/reference example"
	[navigation_grid]="non-visual navigation/reference example"
	[simulation_intro]="non-visual simulation/reference example"
	[simulation_advanced]="non-visual simulation/reference example"
)

usage() {
	cat <<'EOF'
Usage: ./scripts/docs/capture_examples.sh [options] [target...]

Generate local PNG captures for docs example pages.

Options:
  --build                 Build selected targets before capture.
  --frame <n>             Frame to capture (default: 20).
  --timeout <seconds>     Per-example timeout (default: 20).
  --track <name>          default | advanced | all (default: all).
  --show-window           Run with a visible window (default: hidden terminal mode).
  --help                  Show this help.

Examples:
  ./scripts/docs/capture_examples.sh --build
  ./scripts/docs/capture_examples.sh --track default
  ./scripts/docs/capture_examples.sh hello_text model_viewer
EOF
}

declare -a POSITIONAL_TARGETS=()
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
		--track)
			TRACK_FILTER="${2:-}"
			shift 2
			;;
		--show-window)
			USE_TERMINAL=0
			shift
			;;
		--help|-h)
			usage
			exit 0
			;;
		*)
			POSITIONAL_TARGETS+=("$1")
			shift
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
if [[ "$TRACK_FILTER" != "all" && "$TRACK_FILTER" != "default" && "$TRACK_FILTER" != "advanced" ]]; then
	echo "Invalid --track value: $TRACK_FILTER" >&2
	exit 1
fi

declare -A TRACK_BY_TARGET=()
for target in "${DEFAULT_TARGETS[@]}"; do
	TRACK_BY_TARGET["$target"]="default"
done
for target in "${ADVANCED_TARGETS[@]}"; do
	TRACK_BY_TARGET["$target"]="advanced"
done

declare -a CAPTURE_TARGETS=()
if [[ ${#POSITIONAL_TARGETS[@]} -gt 0 ]]; then
	CAPTURE_TARGETS=("${POSITIONAL_TARGETS[@]}")
else
	if [[ "$TRACK_FILTER" == "all" || "$TRACK_FILTER" == "default" ]]; then
		CAPTURE_TARGETS+=("${DEFAULT_TARGETS[@]}")
	fi
	if [[ "$TRACK_FILTER" == "all" || "$TRACK_FILTER" == "advanced" ]]; then
		CAPTURE_TARGETS+=("${ADVANCED_TARGETS[@]}")
	fi
fi

for target in "${CAPTURE_TARGETS[@]}"; do
	if [[ -z "${TRACK_BY_TARGET[$target]+x}" ]]; then
		echo "Unknown target: $target" >&2
		exit 1
	fi
done

if [[ "$SHOULD_BUILD" -eq 1 ]]; then
	for target in "${CAPTURE_TARGETS[@]}"; do
		echo "Building $target"
		"$ROOT_DIR/build.sh" "$target"
	done
fi

mkdir -p "$ROOT_DIR/docs/assets/img/examples/default"
mkdir -p "$ROOT_DIR/docs/assets/img/examples/advanced"

declare -a FAILURES=()
declare -a SUCCESSES=()
declare -a SKIPPED=()

for target in "${CAPTURE_TARGETS[@]}"; do
	if [[ -n "${SKIP_REASON_BY_TARGET[$target]+x}" ]]; then
		echo "Skipping $target (${SKIP_REASON_BY_TARGET[$target]})"
		SKIPPED+=("$target")
		continue
	fi

	track="${TRACK_BY_TARGET[$target]}"
	binary="$ROOT_DIR/bin/$target"
	output="$ROOT_DIR/docs/assets/img/examples/$track/$target.png"

	if [[ ! -x "$binary" ]]; then
		echo "Missing binary: $binary (build first or use --build)" >&2
		FAILURES+=("$target")
		continue
	fi

	rm -f "$output"
	echo "Capturing $target -> ${output#$ROOT_DIR/}"

	declare -a CMD=(
		timeout "${TIMEOUT_SECONDS}s"
		env
		SE_DOCS_CAPTURE_PATH="$output"
		SE_DOCS_CAPTURE_FRAME="$FRAME"
		SE_DOCS_CAPTURE_EXIT=1
	)
	if [[ "$target" == "rts_integration" ]]; then
		CMD=(
			timeout "${TIMEOUT_SECONDS}s"
			env
			SE_DOCS_CAPTURE_PATH="$output"
			SE_DOCS_CAPTURE_FRAME=2
			SE_DOCS_CAPTURE_EXIT=1
		)
	fi
	if [[ "$USE_TERMINAL" -eq 1 ]]; then
		CMD+=(SE_TERMINAL_RENDER=1 SE_TERMINAL_HIDE_WINDOW=1)
	fi
	CMD+=("$binary")

	if [[ "$target" == "rts_integration" ]]; then
		CMD+=(--autotest --seconds=2 --autotest-steps=20 --autotest-dt=0.05)
	fi

	if "${CMD[@]}" >/tmp/se_docs_capture_"$target".log 2>&1; then
		cmd_status=0
	else
		cmd_status=$?
	fi

	if [[ -s "$output" ]]; then
		SUCCESSES+=("$target")
	else
		echo "Capture failed: $target" >&2
		echo "Exit status: $cmd_status" >&2
		tail -n 40 /tmp/se_docs_capture_"$target".log >&2 || true
		FAILURES+=("$target")
	fi
done

echo "Capture summary: ${#SUCCESSES[@]} succeeded, ${#SKIPPED[@]} skipped, ${#FAILURES[@]} failed."
if [[ ${#SUCCESSES[@]} -gt 0 ]]; then
	printf 'Succeeded: %s\n' "${SUCCESSES[*]}"
fi
if [[ ${#SKIPPED[@]} -gt 0 ]]; then
	printf 'Skipped: %s\n' "${SKIPPED[*]}"
fi
if [[ ${#FAILURES[@]} -gt 0 ]]; then
	printf 'Failed: %s\n' "${FAILURES[*]}" >&2
	exit 1
fi
