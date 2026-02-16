#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DOCS_DIR="$ROOT_DIR/docs"
CHECK_EXTERNAL=0

if [[ "${1:-}" == "--external" ]]; then
	CHECK_EXTERNAL=1
fi

python3 - "$DOCS_DIR" "$CHECK_EXTERNAL" <<'PY'
import os
import re
import sys
import urllib.error
import urllib.request
from typing import Dict, List, Set, Tuple


docs_dir = os.path.abspath(sys.argv[1])
check_external = int(sys.argv[2]) == 1

link_pattern = re.compile(r"(!)?\[[^\]]*\]\(([^)]+)\)")
heading_pattern = re.compile(r"^(#{1,6})\s+(.+?)\s*$")


def slugify(text: str) -> str:
    text = text.strip().lower()
    text = re.sub(r"`", "", text)
    text = re.sub(r"[^a-z0-9 _-]", "", text)
    text = re.sub(r"\s+", "-", text)
    text = re.sub(r"-+", "-", text)
    return text.strip("-")


def headings_for_file(path: str) -> Set[str]:
    anchors: Set[str] = set()
    counts: Dict[str, int] = {}
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            m = heading_pattern.match(raw.rstrip("\n"))
            if not m:
                continue
            base = slugify(m.group(2))
            if not base:
                continue
            count = counts.get(base, 0)
            anchor = base if count == 0 else f"{base}-{count}"
            counts[base] = count + 1
            anchors.add(anchor)
    return anchors


def normalize_target(raw_target: str) -> str:
    target = raw_target.strip()
    if " " in target and not target.startswith("<"):
        target = target.split(" ", 1)[0]
    target = target.strip("<>")
    return target


def resolve_local_link(src_file: str, target: str) -> Tuple[str, str]:
    path_part, _, anchor = target.partition("#")

    if path_part.startswith("/"):
        resolved = os.path.join(docs_dir, path_part.lstrip("/"))
    elif path_part:
        resolved = os.path.join(os.path.dirname(src_file), path_part)
    else:
        resolved = src_file

    if os.path.isdir(resolved):
        resolved = os.path.join(resolved, "index.md")

    if path_part and not os.path.splitext(resolved)[1]:
        md_candidate = resolved + ".md"
        index_candidate = os.path.join(resolved, "index.md")
        if os.path.exists(md_candidate):
            resolved = md_candidate
        elif os.path.exists(index_candidate):
            resolved = index_candidate

    resolved = os.path.normpath(resolved)
    return resolved, anchor


all_markdown_files: List[str] = []
for root, _, files in os.walk(docs_dir):
    for file_name in files:
        if file_name.endswith(".md"):
            all_markdown_files.append(os.path.join(root, file_name))

all_markdown_files.sort()

anchors_cache = {path: headings_for_file(path) for path in all_markdown_files}
errors: List[str] = []
checked_external: Set[str] = set()

for src_file in all_markdown_files:
    with open(src_file, "r", encoding="utf-8") as f:
        content = f.read()

    for match in link_pattern.finditer(content):
        is_image = bool(match.group(1))
        target = normalize_target(match.group(2))
        if not target:
            continue
        if is_image:
            # image links are validated only as file paths below
            pass

        if target.startswith("mailto:"):
            continue
        if target.startswith("http://") or target.startswith("https://"):
            if check_external and target not in checked_external:
                checked_external.add(target)
                try:
                    req = urllib.request.Request(target, method="HEAD", headers={"User-Agent": "syphax-docs-linkcheck"})
                    with urllib.request.urlopen(req, timeout=10) as resp:
                        if resp.status >= 400:
                            errors.append(f"{os.path.relpath(src_file, docs_dir)} -> external link failed: {target} (status {resp.status})")
                except urllib.error.HTTPError as ex:
                    if ex.code >= 400:
                        errors.append(f"{os.path.relpath(src_file, docs_dir)} -> external link failed: {target} (status {ex.code})")
                except Exception as ex:
                    errors.append(f"{os.path.relpath(src_file, docs_dir)} -> external link failed: {target} ({ex})")
            continue

        local_path, anchor = resolve_local_link(src_file, target)
        rel_src = os.path.relpath(src_file, docs_dir)

        if not os.path.exists(local_path):
            errors.append(f"{rel_src} -> missing file: {target}")
            continue

        if anchor:
            file_anchors = anchors_cache.get(local_path)
            if file_anchors is None:
                file_anchors = headings_for_file(local_path)
                anchors_cache[local_path] = file_anchors
            if anchor not in file_anchors:
                rel_target = os.path.relpath(local_path, docs_dir)
                errors.append(f"{rel_src} -> missing anchor: {target} (resolved {rel_target}#{anchor})")

if errors:
    print("Link check failed:")
    for err in errors:
        print(f" - {err}")
    raise SystemExit(1)

print(f"Link check passed across {len(all_markdown_files)} markdown files.")
PY
