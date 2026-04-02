#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SITE_DIR="$ROOT_DIR/site"
MARKER="<!-- AUTO-GENERATED FILE: scripts/docs/generate_html_redirects.sh -->"

if [[ ! -d "$SITE_DIR" ]]; then
	echo "Site directory does not exist: $SITE_DIR" >&2
	echo "Run 'mkdocs build --strict --config-file mkdocs.yml' first." >&2
	exit 1
fi

python3 - "$SITE_DIR" "$MARKER" <<'PY'
import html
import sys
from pathlib import Path

site_dir = Path(sys.argv[1]).resolve()
marker = sys.argv[2]

template = """<!doctype html>
{marker}
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="0; url={target}">
    <link rel="canonical" href="{target}">
    <script>
      location.replace(new URL({target_js}, location.href).toString());
    </script>
    <title>Redirecting...</title>
  </head>
  <body>
    <p>Redirecting to <a href="{target}">{target}</a>.</p>
  </body>
</html>
"""

generated = 0
for index_html in sorted(site_dir.rglob("index.html")):
    rel = index_html.relative_to(site_dir)
    if rel == Path("index.html"):
        continue

    page_dir = index_html.parent
    shim_path = page_dir.with_suffix(".html")
    target = f"{page_dir.name}/"

    if shim_path.exists():
        existing = shim_path.read_text(encoding="utf-8")
        if marker not in existing:
            # MkDocs can emit standalone HTML pages such as 404.html.
            # Keep those authoritative and skip generating a redirect shim.
            continue

    shim_path.write_text(
        template.format(
            marker=marker,
            target=html.escape(target, quote=True),
            target_js=target.__repr__(),
        ),
        encoding="utf-8",
    )
    generated += 1

print(f"Generated {generated} HTML redirect shims.")
PY
