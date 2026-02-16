#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "$ROOT_DIR"
./scripts/docs/generate_api_reference.sh
mkdocs serve --config-file mkdocs.yml
