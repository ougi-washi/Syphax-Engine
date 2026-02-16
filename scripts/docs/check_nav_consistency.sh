#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

python3 - "$ROOT_DIR" <<'PY'
import glob
import os
import re
import sys

root_dir = os.path.abspath(sys.argv[1])
docs_dir = os.path.join(root_dir, "docs")
mkdocs_file = os.path.join(root_dir, "mkdocs.yml")

if not os.path.isfile(mkdocs_file):
    raise SystemExit("mkdocs.yml not found")

with open(mkdocs_file, "r", encoding="utf-8") as f:
    mkdocs_text = f.read()

nav_paths = set(re.findall(r"([A-Za-z0-9_./-]+\.md)", mkdocs_text))

all_docs = []
for root, _, files in os.walk(docs_dir):
    for name in files:
        if name.endswith(".md"):
            rel = os.path.relpath(os.path.join(root, name), docs_dir).replace("\\", "/")
            all_docs.append(rel)
all_docs.sort()

excluded_from_nav = {
    "404.md",
}
for path in glob.glob(os.path.join(docs_dir, "templates", "*.md")):
    excluded_from_nav.add(os.path.relpath(path, docs_dir).replace("\\", "/"))

missing_in_nav = [p for p in all_docs if p not in excluded_from_nav and p not in nav_paths]
if missing_in_nav:
    print("Pages missing from mkdocs nav:")
    for path in missing_in_nav:
        print(f" - {path}")
    raise SystemExit(1)

missing_pages = [p for p in sorted(nav_paths) if not os.path.isfile(os.path.join(docs_dir, p))]
if missing_pages:
    print("mkdocs nav references missing files:")
    for path in missing_pages:
        print(f" - {path}")
    raise SystemExit(1)

# API coverage: every public header needs a generated API page and nav entry.
headers = sorted(glob.glob(os.path.join(root_dir, "include", "se_*.h")))
headers += sorted(glob.glob(os.path.join(root_dir, "include", "loader", "*.h")))
if not headers:
    raise SystemExit("No public headers found for API coverage check.")


def header_to_module_id(path: str) -> str:
    rel = os.path.relpath(path, root_dir).replace("\\", "/")
    stem = rel.replace("include/", "").replace(".h", "")
    if stem.startswith("loader/"):
        return "loader_" + stem.split("/", 1)[1]
    return stem

for header in headers:
    module_id = header_to_module_id(header)
    page_rel = f"api-reference/modules/{module_id}.md"
    page_abs = os.path.join(docs_dir, page_rel)
    if not os.path.isfile(page_abs):
        raise SystemExit(f"Missing generated API page for header {header}: {page_rel}")
    if page_rel not in nav_paths:
        raise SystemExit(f"Generated API page missing in nav: {page_rel}")

# Required concept sections
required_sections = {
    "when to use this",
    "minimal working snippet",
    "step-by-step explanation",
    "try this next",
    "common mistakes",
    "related pages",
}

concept_files = []
for folder in ("start-here", "path", "module-guides"):
    folder_abs = os.path.join(docs_dir, folder)
    for path in glob.glob(os.path.join(folder_abs, "*.md")):
        name = os.path.basename(path)
        if name == "index.md":
            continue
        concept_files.append(path)

heading_re = re.compile(r"^##\s+(.+?)\s*$")
for concept in sorted(concept_files):
    found = set()
    with open(concept, "r", encoding="utf-8") as f:
        for line in f:
            m = heading_re.match(line.strip())
            if m:
                heading = m.group(1).strip().lower()
                heading = heading.replace("`", "")
                found.add(heading)

    missing = sorted(required_sections - found)
    if missing:
        rel = os.path.relpath(concept, docs_dir).replace("\\", "/")
        print(f"Missing required sections in concept page: {rel}")
        for heading in missing:
            print(f" - {heading}")
        raise SystemExit(1)

# Duplicate API symbol anchors inside each generated API page.
anchor_re = re.compile(r"^###\s+`([^`]+)`\s*$")

def slugify(value: str) -> str:
    value = value.strip().lower()
    value = re.sub(r"[^a-z0-9 _-]", "", value)
    value = re.sub(r"\s+", "-", value)
    value = re.sub(r"-+", "-", value)
    return value.strip("-")

api_pages = sorted(glob.glob(os.path.join(docs_dir, "api-reference", "modules", "*.md")))
for page in api_pages:
    if os.path.basename(page) == "index.md":
        continue
    seen = set()
    with open(page, "r", encoding="utf-8") as f:
        for line in f:
            m = anchor_re.match(line.strip())
            if not m:
                continue
            symbol = m.group(1)
            anchor = slugify(symbol)
            if anchor in seen:
                rel = os.path.relpath(page, docs_dir).replace("\\", "/")
                raise SystemExit(f"Duplicate API anchor '{anchor}' in {rel}")
            seen.add(anchor)

print(f"Navigation and API consistency checks passed ({len(all_docs)} docs pages).")
PY
