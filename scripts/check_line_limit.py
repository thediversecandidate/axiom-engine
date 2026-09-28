#!/usr/bin/env python3
"""300-line limit (CONTEXT_TOOLING §3).

usage: check_line_limit.py [--root DIR] [--limit N]

Counts physical lines of .hpp .h .cpp .glsl .vert .frag .comp .py files, excluding build/,
_deps/, .git/ and scripts/tests/fixtures/. Diagnostic: AX-LEN-001.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

EXTENSIONS = {".hpp", ".h", ".cpp", ".glsl", ".vert", ".frag", ".comp", ".py"}
EXCLUDED_PARTS = {"build", "_deps", ".git"}
EXCLUDED_PREFIX = ("scripts", "tests", "fixtures")


def excluded(rel: Path) -> bool:
    return bool(EXCLUDED_PARTS & set(rel.parts)) or rel.parts[:3] == EXCLUDED_PREFIX


def check(root: Path, limit: int) -> list[str]:
    problems = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix not in EXTENSIONS:
            continue
        rel = path.relative_to(root)
        if excluded(rel):
            continue
        lines = len(path.read_bytes().splitlines())
        if lines > limit:
            problems.append(f"AX-LEN-001 {rel}: {lines} lines (limit {limit})")
    return problems


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--limit", type=int, default=300)
    args = parser.parse_args(argv)
    problems = check(args.root, args.limit)
    for p in problems:
        print(p)
    print(f"check_line_limit: {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
