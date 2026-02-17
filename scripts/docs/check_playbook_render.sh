#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SITE_PLAYBOOKS_DIR="$ROOT_DIR/site/playbooks"

if [[ ! -d "$SITE_PLAYBOOKS_DIR" ]]; then
	echo "Missing built playbooks directory: $SITE_PLAYBOOKS_DIR" >&2
	echo "Run 'mkdocs build --strict --config-file mkdocs.yml' first." >&2
	exit 1
fi

python3 - "$SITE_PLAYBOOKS_DIR" <<'PY'
import re
import sys
from pathlib import Path

playbooks_dir = Path(sys.argv[1]).resolve()
errors = []

for page in sorted(playbooks_dir.glob("*/index.html")):
    html = page.read_text(encoding="utf-8")
    code_blocks = re.findall(
        r'<div class="highlight"><pre><span></span><code>(.*?)</code></pre></div>',
        html,
        flags=re.S,
    )
    step_blocks = [block for block in code_blocks if "__codelineno-" in block]
    if len(step_blocks) < 4:
        errors.append(f"{page}: expected at least 4 rendered code blocks, found {len(step_blocks)}")
        continue

    for idx, block in enumerate(step_blocks[:4], start=1):
        text = re.sub(r"<[^>]+>", "", block)
        text = (
            text.replace("&quot;", "\"")
            .replace("&lt;", "<")
            .replace("&gt;", ">")
            .replace("&amp;", "&")
        )
        non_empty_lines = [line.strip() for line in text.splitlines() if line.strip()]
        if len(non_empty_lines) < 3:
            errors.append(f"{page}: step {idx} snippet rendered empty")

if errors:
    print("Playbook render checks failed:")
    for error in errors:
        print(f" - {error}")
    raise SystemExit(1)

print("Playbook render checks passed.")
PY
