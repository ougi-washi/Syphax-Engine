#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="$ROOT_DIR/docs/api-reference/modules"
MARKER="<!-- AUTO-GENERATED FILE: scripts/docs/generate_api_reference.sh -->"

if [[ ! -d "$ROOT_DIR/include" ]]; then
	echo "Missing include directory at $ROOT_DIR/include" >&2
	exit 1
fi
if [[ ! -d "$ROOT_DIR/include/loader" ]]; then
	echo "Missing include/loader directory at $ROOT_DIR/include/loader" >&2
	exit 1
fi

mkdir -p "$OUT_DIR"

python3 - "$ROOT_DIR" "$OUT_DIR" "$MARKER" <<'PY'
import glob
import os
import re
import sys
from dataclasses import dataclass
from typing import List, Tuple

root_dir = os.path.abspath(sys.argv[1])
out_dir = os.path.abspath(sys.argv[2])
marker = sys.argv[3]

header_paths = sorted(glob.glob(os.path.join(root_dir, "include", "se_*.h")))
header_paths += sorted(glob.glob(os.path.join(root_dir, "include", "loader", "*.h")))
if not header_paths:
    raise SystemExit("No source headers found in include/se_*.h or include/loader/*.h")

for header in header_paths:
    if not os.path.isfile(header):
        raise SystemExit(f"Missing source header: {header}")


@dataclass
class Declaration:
    name: str
    signature: str
    comment: str


@dataclass
class ModuleDoc:
    header_rel: str
    module_id: str
    title: str
    functions: List[Declaration]
    enums: List[Declaration]
    typedefs: List[Declaration]


MODULE_OVERVIEW = {
    "se_audio": "Audio engine setup, clips, streams, buses, and capture APIs.",
    "se_backend": "Backend selection and runtime backend information.",
    "se_camera": "Camera data, projection setup, and camera-controller helpers.",
    "se_curve": "Curve keyframe data and interpolation helpers.",
    "se_debug": "Logging, trace spans, overlays, and frame timing diagnostics.",
    "se_defines": "Global constants, resource scope macros, and engine-wide result codes.",
    "se_ext": "Optional extension capability checks.",
    "se_framebuffer": "Framebuffer creation, binding, and resize lifecycle.",
    "se_graphics": "Render lifecycle helpers such as clear, blending, and context state.",
    "se_input": "Input bindings, action contexts, queues, recording, and replay.",
    "se_math": "Geometry utility structs and math helpers used by engine modules.",
    "se_model": "Model loading, model handles, and mesh composition APIs.",
    "se_navigation": "Grid navigation, pathfinding, smoothing, and occupancy controls.",
    "se_physics": "2D and 3D rigid body worlds, shapes, stepping, and queries.",
    "se_quad": "Quad and mesh instance utility structs.",
    "se_render_buffer": "Render-to-texture buffers and offscreen draw orchestration.",
    "se_scene": "2D/3D scenes, objects, instance management, and picking.",
    "se_sdf": "Signed distance field scene graph, renderer, and material controls.",
    "se_shader": "Shader loading, uniform access, and shader handle lifecycle.",
    "se_simulation": "Deterministic simulation components, systems, events, and snapshots.",
    "se_text": "Font loading and text drawing APIs.",
    "se_texture": "Texture loading from files or memory and texture parameter control.",
    "se_ui": "Immediate mode style UI tree, widgets, layout, and events.",
    "se_vfx": "Particle system APIs for 2D and 3D emitters.",
    "se_window": "Window lifecycle, frame loop, input state, and cursor/window utilities.",
    "loader_se_gltf": "GLTF/GLB parsing, in-memory representation, and export utilities.",
    "loader_se_loader": "General loader helpers used by model and asset loading paths.",
}

PATH_WALKTHROUGH = {
    "se_window": "path/window.md",
    "se_input": "path/input.md",
    "se_camera": "path/camera.md",
    "se_scene": "path/scene.md",
    "se_ui": "path/ui.md",
    "se_audio": "path/audio.md",
    "se_physics": "path/physics.md",
    "se_vfx": "path/vfx.md",
    "se_navigation": "path/navigation.md",
    "se_simulation": "path/simulation.md",
    "se_debug": "path/debug.md",
    "se_graphics": "path/graphics.md",
    "se_model": "path/model.md",
    "se_shader": "path/shader.md",
    "se_texture": "path/texture.md",
    "se_text": "path/text.md",
    "se_render_buffer": "path/render-buffer.md",
    "se_framebuffer": "path/framebuffer.md",
    "se_backend": "path/backend.md",
    "se_curve": "path/curve.md",
    "se_sdf": "path/sdf.md",
    "loader_se_loader": "path/loader.md",
    "loader_se_gltf": "path/gltf.md",
    "se_math": "path/utilities.md",
    "se_defines": "path/utilities.md",
    "se_ext": "path/utilities.md",
    "se_quad": "path/utilities.md",
}


def normalize_comment_line(text: str) -> str:
    text = text.strip()
    text = re.sub(r"^/\*+", "", text)
    text = re.sub(r"\*+/$", "", text)
    text = re.sub(r"^\*", "", text)
    return text.strip()


def parse_top_level_declarations(source: str) -> List[Tuple[str, str]]:
    lines = source.splitlines()
    declarations = []
    pending_comment: List[str] = []
    buffer: List[str] = []
    brace_depth = 0
    in_block_comment = False

    def flush_buffer() -> None:
        nonlocal buffer, pending_comment, brace_depth
        if not buffer:
            return
        declaration = " ".join(part.strip() for part in buffer if part.strip())
        declaration = re.sub(r"\s+", " ", declaration).strip()
        comment = " ".join(part.strip() for part in pending_comment if part.strip())
        comment = re.sub(r"\s+", " ", comment).strip()
        if declaration:
            declarations.append((declaration, comment))
        buffer = []
        pending_comment = []
        brace_depth = 0

    for raw_line in lines:
        line = raw_line.rstrip("\n")
        stripped = line.strip()

        if in_block_comment:
            end_idx = stripped.find("*/")
            if end_idx >= 0:
                prefix = stripped[:end_idx]
                cleaned = normalize_comment_line(prefix)
                if cleaned:
                    pending_comment.append(cleaned)
                in_block_comment = False
            else:
                cleaned = normalize_comment_line(stripped)
                if cleaned:
                    pending_comment.append(cleaned)
            continue

        if not buffer:
            if not stripped:
                pending_comment = []
                continue
            if stripped.startswith("//"):
                pending_comment.append(stripped[2:].strip())
                continue
            if stripped.startswith("/*"):
                end_idx = stripped.find("*/")
                if end_idx >= 0:
                    inner = stripped[2:end_idx]
                    cleaned = normalize_comment_line(inner)
                    if cleaned:
                        pending_comment.append(cleaned)
                    continue
                cleaned = normalize_comment_line(stripped[2:])
                if cleaned:
                    pending_comment.append(cleaned)
                in_block_comment = True
                continue
            if stripped.startswith("#"):
                pending_comment = []
                continue

        buffer.append(line)
        brace_depth += line.count("{") - line.count("}")
        if ";" in line and brace_depth == 0:
            flush_buffer()

    flush_buffer()
    return declarations


def parse_function_name(signature: str) -> str:
    m = re.search(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*;$", signature)
    return m.group(1) if m else "unknown_function"


def parse_typedef_name(signature: str) -> str:
    func_ptr = re.search(r"\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(", signature)
    if func_ptr:
        return func_ptr.group(1)
    block_alias = re.search(r"}\s*([A-Za-z_][A-Za-z0-9_]*)\s*;$", signature)
    if block_alias:
        return block_alias.group(1)
    simple = re.search(r"typedef\s+.*?\b([A-Za-z_][A-Za-z0-9_]*)\s*;$", signature)
    if simple:
        return simple.group(1)
    return "unknown_typedef"


def parse_enum_name(signature: str) -> str:
    alias = re.search(r"}\s*([A-Za-z_][A-Za-z0-9_]*)\s*;$", signature)
    if alias:
        return alias.group(1)
    named = re.search(r"enum\s+([A-Za-z_][A-Za-z0-9_]*)", signature)
    if named:
        return named.group(1)
    return "unknown_enum"


def classify_declarations(declarations: List[Tuple[str, str]]) -> Tuple[List[Declaration], List[Declaration], List[Declaration]]:
    functions: List[Declaration] = []
    enums: List[Declaration] = []
    typedefs: List[Declaration] = []

    for signature, comment in declarations:
        if signature.startswith("extern "):
            name = parse_function_name(signature)
            functions.append(Declaration(name=name, signature=signature, comment=comment))
            continue
        if signature.startswith("typedef enum") or signature.startswith("enum "):
            name = parse_enum_name(signature)
            enums.append(Declaration(name=name, signature=signature, comment=comment))
            continue
        if signature.startswith("typedef "):
            name = parse_typedef_name(signature)
            typedefs.append(Declaration(name=name, signature=signature, comment=comment))
            continue

    functions.sort(key=lambda d: (d.name, d.signature))
    enums.sort(key=lambda d: (d.name, d.signature))
    typedefs.sort(key=lambda d: (d.name, d.signature))

    def uniquify(items: List[Declaration], unknown_prefix: str) -> List[Declaration]:
        seen = {}
        unique: List[Declaration] = []
        for item in items:
            base = item.name.strip() if item.name else unknown_prefix
            if base.startswith("unknown_"):
                base = unknown_prefix

            count = seen.get(base, 0)
            seen[base] = count + 1
            if count == 0:
                unique_name = base
            else:
                unique_name = f"{base}_{count + 1}"

            unique.append(
                Declaration(
                    name=unique_name,
                    signature=item.signature,
                    comment=item.comment,
                )
            )
        return unique

    functions = uniquify(functions, "function")
    enums = uniquify(enums, "enum")
    typedefs = uniquify(typedefs, "typedef")
    return functions, enums, typedefs


def module_id_for_header(header_rel: str) -> str:
    stem = header_rel.replace("include/", "").replace(".h", "")
    if stem.startswith("loader/"):
        return "loader_" + stem.split("/", 1)[1]
    return stem


def title_for_module(module_id: str, header_rel: str) -> str:
    if module_id.startswith("loader_"):
        return header_rel.replace("include/", "")
    return f"{module_id}.h"


def render_declaration_section(kind: str, items: List[Declaration]) -> List[str]:
    lines: List[str] = [f"## {kind}", ""]
    if not items:
        lines.append(f"No {kind.lower()} found in this header.")
        lines.append("")
        return lines

    for item in items:
        lines.append(f"### `{item.name}`")
        lines.append("")
        lines.append('<div class="api-signature">')
        lines.append("")
        lines.append("```c")
        lines.append(item.signature)
        lines.append("```")
        lines.append("")
        lines.append("</div>")
        lines.append("")
        if item.comment:
            cleaned_comment = (
                item.comment
                .replace("beginner", "default")
                .replace("Beginner", "Default")
                .replace("artist", "user")
                .replace("Artist", "User")
            )
            lines.append(cleaned_comment)
        else:
            lines.append("No inline description found in header comments.")
        lines.append("")
    return lines


def write_markdown(path: str, lines: List[str]) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    content = "\n".join(lines).rstrip() + "\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


existing_files = sorted(glob.glob(os.path.join(out_dir, "*.md")))
for existing in existing_files:
    with open(existing, "r", encoding="utf-8") as f:
        first_line = f.readline().strip()
    if first_line and first_line != marker:
        raise SystemExit(
            f"Refusing to overwrite non-generated file: {existing}. "
            "Add the generated marker or move this file out of docs/api-reference/modules/."
        )

module_docs: List[ModuleDoc] = []
for header_path in header_paths:
    header_rel = os.path.relpath(header_path, root_dir).replace("\\", "/")
    with open(header_path, "r", encoding="utf-8") as f:
        source = f.read()

    declarations = parse_top_level_declarations(source)
    functions, enums, typedefs = classify_declarations(declarations)

    module_id = module_id_for_header(header_rel)
    title = title_for_module(module_id, header_rel)
    module_docs.append(
        ModuleDoc(
            header_rel=header_rel,
            module_id=module_id,
            title=title,
            functions=functions,
            enums=enums,
            typedefs=typedefs,
        )
    )

module_docs.sort(key=lambda d: d.module_id)
expected_files = set()

for module in module_docs:
    out_file = os.path.join(out_dir, f"{module.module_id}.md")
    expected_files.add(os.path.abspath(out_file))

    overview = MODULE_OVERVIEW.get(module.module_id, "Public declarations exposed by this header.")
    lines = [
        marker,
        f"# {module.title}",
        "",
        f"Source header: `{module.header_rel}`",
        "",
        "## Overview",
        "",
        overview,
        "",
        f"This page is generated from `{module.header_rel}` and is deterministic.",
        "",
    ]

    path_rel = PATH_WALKTHROUGH.get(module.module_id)
    if path_rel:
        lines.extend(
            [
                "## Read the path walkthrough",
                "",
                f"- [Deep dive path page](../../{path_rel})",
                "",
            ]
        )

    lines.extend(
        [
        *render_declaration_section("Functions", module.functions),
        *render_declaration_section("Enums", module.enums),
        *render_declaration_section("Typedefs", module.typedefs),
        ]
    )
    write_markdown(out_file, lines)

index_path = os.path.join(out_dir, "index.md")
expected_files.add(os.path.abspath(index_path))
index_lines = [
    marker,
    "# API Module Index",
    "",
    "Generated API pages by public header.",
    "",
    "| Header | Functions | Enums | Typedefs |",
    "| --- | ---: | ---: | ---: |",
]
for module in module_docs:
    rel_path = f"{module.module_id}.md"
    index_lines.append(
        f"| [{module.header_rel}]({rel_path}) | {len(module.functions)} | {len(module.enums)} | {len(module.typedefs)} |"
    )

index_lines.extend(
    [
        "",
        "All files in this folder are generated by `scripts/docs/generate_api_reference.sh`.",
        "",
    ]
)
write_markdown(index_path, index_lines)

for existing in existing_files:
    abs_existing = os.path.abspath(existing)
    if abs_existing not in expected_files:
        os.remove(existing)

print(f"Generated API reference for {len(module_docs)} modules.")
PY
