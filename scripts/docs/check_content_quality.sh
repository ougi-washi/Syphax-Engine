#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DOCS_DIR="$ROOT_DIR/docs"

python3 - "$DOCS_DIR" <<'PY'
import os
import re
import sys


docs_dir = os.path.abspath(sys.argv[1])
heading_re = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
link_re = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")
ambiguous = {"here", "click here", "this", "link"}
errors = []

for root, _, files in os.walk(docs_dir):
    for name in files:
        if not name.endswith('.md'):
            continue
        path = os.path.join(root, name)
        rel = os.path.relpath(path, docs_dir).replace("\\", "/")
        with open(path, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        last_level = 0
        for idx, raw in enumerate(lines, start=1):
            line = raw.rstrip('\n')
            m = heading_re.match(line)
            if m:
                level = len(m.group(1))
                if last_level and level > last_level + 1:
                    errors.append(f"{rel}:{idx} heading level jumps from H{last_level} to H{level}")
                last_level = level

            for lm in link_re.finditer(line):
                text = lm.group(1).strip().lower()
                if text in ambiguous:
                    errors.append(f"{rel}:{idx} ambiguous link text '{lm.group(1)}'")

if errors:
    print("Content quality checks failed:")
    for err in errors:
        print(f" - {err}")
    raise SystemExit(1)

print("Content quality checks passed.")
PY
