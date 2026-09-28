#!/usr/bin/env python3
"""Module dependency checker (CONTEXT_RULES §1).

usage: check_includes.py [--root DIR]

Diagnostics:
  AX-INC-001  a module includes a header of a module it may not depend on
  AX-INC-002  a relative include escapes into another module
  AX-INC-003  a module includes another module's private src/ file
  AX-INC-004  a forbidden module is reached transitively through an include chain
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ALLOWED = {
    "core": set(),
    "math": set(),
    "physics": {"core", "math"},
    "renderer": {"core", "math"},
    "ai": {"core", "math"},
    "app": {"core", "math", "physics", "renderer", "ai"},
}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
SOURCE_EXT = {".hpp", ".h", ".cpp"}


def module_of(path: Path, modules_dir: Path) -> str | None:
    try:
        return path.resolve().relative_to(modules_dir.resolve()).parts[0]
    except (ValueError, IndexError):
        return None


def resolve(spec: str, including: Path, modules_dir: Path) -> Path | None:
    """Returns the file an include refers to, if it is inside modules/."""
    candidate = (including.parent / spec).resolve()
    if spec.startswith(".") and candidate.exists():
        return candidate
    match = re.match(r"axiom/([^/]+)/(.+)", spec)
    if match:
        target = modules_dir / match.group(1) / "include" / "axiom" / match.group(1) / match.group(2)
        return target.resolve()
    return candidate if candidate.exists() and module_of(candidate, modules_dir) else None


def direct_includes(path: Path, modules_dir: Path) -> list[tuple[str, Path | None]]:
    text = path.read_text(errors="replace")
    return [(spec, resolve(spec, path, modules_dir)) for spec in INCLUDE_RE.findall(text)]


def check(root: Path) -> list[str]:
    modules_dir = root / "modules"
    problems: list[str] = []
    files = sorted(p for p in modules_dir.rglob("*") if p.suffix in SOURCE_EXT and p.is_file())
    graph = {f.resolve(): direct_includes(f, modules_dir) for f in files}
    for f in files:
        owner = module_of(f, modules_dir)
        if owner not in ALLOWED:
            continue
        allowed = ALLOWED[owner] | {owner}
        rel = f.relative_to(root)
        for spec, target in graph[f.resolve()]:
            if target is None:
                continue
            target_mod = module_of(target, modules_dir)
            if target_mod is None or target_mod == owner:
                continue
            parts = target.relative_to(modules_dir.resolve()).parts
            if len(parts) > 1 and parts[1] == "src":
                problems.append(f"AX-INC-003 {rel}: includes private file of '{target_mod}': {spec}")
            elif spec.startswith("."):
                problems.append(f"AX-INC-002 {rel}: relative include escapes into '{target_mod}': {spec}")
            elif target_mod not in allowed:
                problems.append(f"AX-INC-001 {rel}: '{owner}' may not include '{target_mod}': {spec}")
        # transitive closure through resolved headers
        seen: set[Path] = set()
        stack = [t for _, t in graph[f.resolve()] if t is not None]
        while stack:
            node = stack.pop()
            if node in seen:
                continue
            seen.add(node)
            node_mod = module_of(node, modules_dir)
            if node_mod and node_mod not in allowed:
                problems.append(f"AX-INC-004 {rel}: reaches forbidden module '{node_mod}' via {node.name}")
                break
            stack.extend(t for _, t in graph.get(node, []) if t is not None)
    return sorted(set(problems))


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args(argv)
    problems = check(args.root)
    for p in problems:
        print(p)
    print(f"check_includes: {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
