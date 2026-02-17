#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

python3 - "$ROOT_DIR" <<'PY'
import os
import re
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve()
docs = root / "docs"
playbooks_dir = docs / "playbooks"
mkdocs_path = root / "mkdocs.yml"

required_playbooks = [
    "se-window.md",
    "se-input.md",
    "se-camera.md",
    "se-scene.md",
    "se-ui.md",
    "se-audio.md",
    "se-physics.md",
    "se-vfx.md",
    "se-navigation.md",
    "se-simulation.md",
    "se-debug.md",
    "se-graphics.md",
    "se-model.md",
    "se-shader.md",
    "se-texture.md",
    "se-text.md",
    "se-render-buffer.md",
    "se-framebuffer.md",
    "se-backend.md",
    "se-curve.md",
    "se-sdf.md",
    "loader-se-loader.md",
    "loader-se-gltf.md",
    "se-math-defines-ext-quad.md",
]

required_headings = {
    "when to use this",
    "step 1: minimal working project",
    "step 2: add core feature",
    "step 3: interactive / tunable",
    "step 4: complete practical demo",
    "common mistakes",
    "next",
    "related pages",
}

missing_files = []
for name in required_playbooks:
    if not (playbooks_dir / name).is_file():
        missing_files.append(name)
if missing_files:
    print("Missing required Playbook pages:")
    for name in missing_files:
        print(f" - {name}")
    raise SystemExit(1)

heading_re = re.compile(r"^##\s+(.+?)\s*$")
snippet_include_re = re.compile(r'--8<--\s+"(snippets/[^"]+\.c)"')
api_link_re = re.compile(r"\(\.\./api-reference/modules/[A-Za-z0-9_./-]+\.md\)")
example_link_re = re.compile(r"\(\.\./examples/[A-Za-z0-9_./-]+\.md\)")

errors = []
mkdocs_text = mkdocs_path.read_text(encoding="utf-8")
if "pymdownx.snippets:" not in mkdocs_text:
    errors.append("mkdocs.yml: missing pymdownx.snippets configuration block")
if "check_paths: true" not in mkdocs_text:
    errors.append("mkdocs.yml: pymdownx.snippets must set check_paths: true")
if "base_path:" not in mkdocs_text or "\n        - docs\n" not in mkdocs_text:
    errors.append("mkdocs.yml: pymdownx.snippets must include base_path with docs")

for name in required_playbooks:
    path = playbooks_dir / name
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()

    found = set()
    for line in lines:
        m = heading_re.match(line.strip())
        if m:
            found.add(m.group(1).strip().lower().replace("`", ""))

    missing = sorted(required_headings - found)
    if missing:
        errors.append(f"{name}: missing required headings: {', '.join(missing)}")

    includes = snippet_include_re.findall(text)
    if len(includes) != 4:
        errors.append(f"{name}: expected 4 snippet includes, found {len(includes)}")
    for include in includes:
        snippet_path = docs / include
        if not snippet_path.is_file():
            errors.append(f"{name}: missing snippet file {include}")
        elif snippet_path.stat().st_size == 0:
            errors.append(f"{name}: snippet file is empty: {include}")

    if "Key API calls:\n- " in text:
        errors.append(f"{name}: missing blank line after 'Key API calls:'")
    if len(re.findall(r"Key API calls:\n\n- ", text)) != 4:
        errors.append(f"{name}: expected 4 well-formed 'Key API calls' list blocks")

    if not api_link_re.search(text):
        errors.append(f"{name}: missing API module link")
    if not example_link_re.search(text):
        errors.append(f"{name}: missing example link")

if errors:
    print("Playbook coverage checks failed:")
    for err in errors:
        print(f" - {err}")
    raise SystemExit(1)

print(f"Playbook coverage checks passed ({len(required_playbooks)} required playbooks).")
PY
