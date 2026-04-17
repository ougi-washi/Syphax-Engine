#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

python3 - "$ROOT_DIR" <<'PY'
import re
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve()
docs = root / "docs"
mkdocs_path = root / "mkdocs.yml"
examples_index_path = docs / "examples" / "index.md"

mkdocs_text = mkdocs_path.read_text(encoding="utf-8")
examples_index_text = examples_index_path.read_text(encoding="utf-8")

nav_links = set(re.findall(r"(examples/[A-Za-z0-9_./-]+\.md)", mkdocs_text))
index_links = set(re.findall(r"\((default|advanced)/([A-Za-z0-9_-]+)\.md\)", examples_index_text))

errors = []
documented_targets = set()

tracks = {
    "default": root / "examples",
    "advanced": root / "examples" / "advanced",
}

for track, source_dir in tracks.items():
    source_paths = sorted(source_dir.glob("*.c"))
    for source_path in source_paths:
        target = source_path.stem
        documented_targets.add((track, target))
        page_rel = f"examples/{track}/{target}.md"
        page_path = docs / page_rel
        if not page_path.is_file():
            errors.append(f"missing docs page for target {track}/{target}: {page_rel}")
        if page_rel not in nav_links:
            errors.append(f"mkdocs nav missing example page: {page_rel}")
        if (track, target) not in index_links:
            errors.append(f"docs/examples/index.md missing link for target {track}/{target}")

for track in tracks:
    docs_dir = docs / "examples" / track
    for page_path in sorted(docs_dir.glob("*.md")):
        target = page_path.stem
        if (track, target) not in documented_targets:
            errors.append(f"example page has no matching source target: examples/{track}/{target}.md")

if errors:
    print("Example coverage checks failed:")
    for error in errors:
        print(f" - {error}")
    raise SystemExit(1)

print(f"Example coverage checks passed ({len(documented_targets)} runnable targets documented).")
PY
