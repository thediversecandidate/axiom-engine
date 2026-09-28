#!/usr/bin/env python3
"""Repository maps (CONTEXT_TOOLING §2): one map per module + an index, from public headers.

usage: gen_repo_map.py [--root DIR] [--out DIR] [--check]

Each map lists every public declaration's signature and its /// contract lines. Maps are never
truncated. Diagnostics:
  AX-MAP-001  tree-sitter parse error in a public header
  AX-MAP-002  a map exceeds its token budget (index 1,000; module 2,500)
  AX-MAP-003  committed maps are stale (--check regenerates into a temp dir and diffs)
"""
from __future__ import annotations

import argparse
import filecmp
import re
import sys
import tempfile
from pathlib import Path

import tree_sitter_cpp
from tree_sitter import Language, Parser

sys.path.insert(0, str(Path(__file__).resolve().parent))
from axiom_tokens import load_counter  # noqa: E402
from check_includes import ALLOWED  # noqa: E402

INDEX_BUDGET = 1000
MODULE_BUDGET = 2500
BODY_TYPES = {"compound_statement", "field_declaration_list", "enumerator_list", "declaration_list"}
DECL_TYPES = {"function_definition", "declaration", "struct_specifier", "class_specifier", "enum_specifier",
              "alias_declaration", "template_declaration", "type_definition", "preproc_function_def",
              "preproc_def"}
MEMBER_TYPES = {"field_declaration", "declaration", "function_definition", "template_declaration"}
PARSER = Parser(Language(tree_sitter_cpp.language()))


def squash(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def signature(node, src: bytes) -> str:
    end = node.end_byte
    for child in node.children:
        if child.type in BODY_TYPES:
            end = child.start_byte
            break
        for grand in child.children:  # e.g. `struct X {...};` wrapped in a declaration
            if grand.type in BODY_TYPES:
                end = grand.start_byte
                break
    return squash(src[node.start_byte:end].decode()).rstrip(";").rstrip()


def contract_lines(node, src: bytes) -> list[str]:
    lines, prev, row = [], node.prev_sibling, node.start_point[0]
    while prev is not None and prev.type == "comment" and prev.end_point[0] >= row - 1:
        text = src[prev.start_byte:prev.end_byte].decode().strip()
        if text.startswith("///"):
            lines.insert(0, text)
        row, prev = prev.start_point[0], prev.prev_sibling
    return lines


def summarize_special(name: str, sigs: list[str]) -> list[str]:
    """Collapses constructors/destructor/assignment boilerplate of class `name` into one line."""
    n = re.escape(name)
    special = re.compile(rf"^(~?{n}\s*\(|{n}\s*&\s*operator=\s*\()")
    kept = [s for s in sigs if not special.search(s)]
    specials = [s for s in sigs if special.search(s)]
    if not specials:
        return sigs
    traits = []
    if any(re.search(rf"^{n}\s*\(\s*const {n}\s*&\s*\)\s*=\s*delete", s) for s in specials):
        traits.append("move-only" if any(re.search(rf"^{n}\s*\(\s*{n}\s*&&", s) for s in specials) else "non-copyable")
    if any(re.search(rf"^{n}\s*\(\s*\)", s) for s in specials):
        traits.append("default-constructible")
    others = [s for s in specials if not re.search(rf"^(~{n}|{n}\s*\(\s*\)|{n}\s*\(\s*(const )?{n}\s*&|{n}\s*&\s*operator=)", s)]
    return ([f"({', '.join(traits)})"] if traits else []) + others + kept


def members(node, src: bytes) -> list[str]:
    name_node = node.child_by_field_name("name")
    name = src[name_node.start_byte:name_node.end_byte].decode() if name_node else ""
    for child in [node] + list(node.children):
        for part in child.children:
            if part.type == "field_declaration_list":
                sigs = [signature(m, src) for m in part.children if m.type in MEMBER_TYPES]
                return summarize_special(name, sigs) if name else sigs
            if part.type == "enumerator_list":
                return [", ".join(squash(src[e.start_byte:e.end_byte].decode())
                                  for e in part.children if e.type == "enumerator")]
    return []


def walk(node, src: bytes, out: list[str], depth: int = 0) -> None:
    for child in node.children:
        if child.type == "namespace_definition":
            body = child.child_by_field_name("body")
            if body is not None:
                walk(body, src, out, depth)
        elif child.type.startswith(("preproc_if", "preproc_else", "preproc_elif")) or \
                child.type == "linkage_specification":
            walk(child, src, out, depth)
        elif child.type in DECL_TYPES:
            sig = signature(child, src)
            if not sig or sig.startswith("#if"):
                continue
            out.append(f"- `{sig}`")
            out.extend(f"  {line}" for line in contract_lines(child, src))
            out.extend(f"  - `{m}`" for m in members(child, src) if m)


def header_purpose(src: bytes) -> str:
    for line in src.decode().splitlines():
        stripped = line.strip()
        if stripped.startswith("//") and not stripped.startswith("///"):
            return stripped.lstrip("/ ").strip()
    return ""


def build_maps(root: Path) -> tuple[dict[str, str], list[str]]:
    modules_dir, problems, maps, index = root / "modules", [], {}, ["# Axiom repository map — index", ""]
    for module in sorted(p.name for p in modules_dir.iterdir() if p.is_dir()):
        headers = sorted((modules_dir / module / "include").rglob("*.hpp"))
        deps = ", ".join(sorted(ALLOWED.get(module, set()))) or "none"
        lines = [f"# Module `{module}` — public API", f"Depends on: {deps}", ""]
        for header in headers:
            src = header.read_bytes()
            tree = PARSER.parse(src)
            rel = header.relative_to(root)
            if tree.root_node.has_error:
                problems.append(f"AX-MAP-001 {rel}: tree-sitter parse error")
            lines.append(f"## `{rel}` — {header_purpose(src)}")
            walk(tree.root_node, src, lines)
            lines.append("")
        maps[f"{module}.md"] = "\n".join(lines).rstrip() + "\n"
        index.append(f"- `{module}` (depends on: {deps}) — {len(headers)} public header(s) → `{module}.md`")
    maps["index.md"] = "\n".join(index) + "\n"
    return maps, problems


def budget_problems(maps: dict[str, str]) -> list[str]:
    counter, problems = load_counter(), []
    for name, text in maps.items():
        budget = INDEX_BUDGET if name == "index.md" else MODULE_BUDGET
        tokens = counter.count(text)
        if tokens > budget:
            problems.append(f"AX-MAP-002 {name}: {tokens} tokens ({counter.method}) exceeds budget {budget}")
    return problems


def write_maps(maps: dict[str, str], out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for name, text in maps.items():
        (out / name).write_text(text)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--out", type=Path, default=None)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    out = args.out or args.root / "docs" / "repomap"
    maps, problems = build_maps(args.root)
    problems += budget_problems(maps)
    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            write_maps(maps, Path(tmp))
            names = sorted(set(maps) | {p.name for p in out.glob("*.md")} if out.exists() else set(maps))
            _, mismatch, errors = filecmp.cmpfiles(tmp, out, names, shallow=False)
            for name in mismatch + errors:
                problems.append(f"AX-MAP-003 {name}: committed map is stale or missing; run gen_repo_map.py")
    elif not problems:
        write_maps(maps, out)
    for p in problems:
        print(p)
    print(f"gen_repo_map: {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
