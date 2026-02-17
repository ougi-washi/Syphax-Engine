#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SITE_PATH_DIR="$ROOT_DIR/site/path"

if [[ ! -d "$SITE_PATH_DIR" ]]; then
	echo "Missing built path directory: $SITE_PATH_DIR" >&2
	echo "Run 'mkdocs build --strict --config-file mkdocs.yml' first." >&2
	exit 1
fi

python3 - "$SITE_PATH_DIR" <<'PY'
import re
import sys
from pathlib import Path

path_dir = Path(sys.argv[1]).resolve()
errors = []

required_slugs = [
    "window",
    "input",
    "ui",
    "text",
    "audio",
    "vfx",
    "curve",
    "sdf",
    "utilities",
    "graphics",
    "camera",
    "scene",
    "model",
    "shader",
    "texture",
    "loader",
    "gltf",
    "render-buffer",
    "framebuffer",
    "backend",
    "physics",
    "navigation",
    "simulation",
    "debug",
]

for slug in required_slugs:
    page = path_dir / slug / "index.html"
    if not page.is_file():
        errors.append(f"missing rendered page: {page}")
        continue

    html = page.read_text(encoding="utf-8")
    code_blocks = re.findall(
        r'<div class="highlight"><pre><span></span><code>(.*?)</code></pre></div>',
        html,
        flags=re.S,
    )
    step_blocks = [block for block in code_blocks if "__codelineno-" in block]
    valid_steps = []
    for block in step_blocks:
        text = re.sub(r"<[^>]+>", "", block)
        text = (
            text.replace("&quot;", "\"")
            .replace("&lt;", "<")
            .replace("&gt;", ">")
            .replace("&amp;", "&")
        )
        non_empty_lines = [line.strip() for line in text.splitlines() if line.strip()]
        if len(non_empty_lines) >= 3:
            valid_steps.append(non_empty_lines)

    if len(valid_steps) < 4:
        errors.append(f"{page}: expected 4 non-empty rendered step snippets, found {len(valid_steps)}")

if errors:
    print("Path render checks failed:")
    for error in errors:
        print(f" - {error}")
    raise SystemExit(1)

print("Path render checks passed.")
PY
