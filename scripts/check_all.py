#!/usr/bin/env python3
"""Runs every non-build Definition-of-Done check (CONTEXT_RULES §5, item 2).

usage: check_all.py [--clang-format BINARY]
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PY = sys.executable
FORMAT_DIRS = ("modules", "tests", "third_party_impl")


def format_targets() -> list[str]:
    return sorted(str(p) for d in FORMAT_DIRS for p in (ROOT / d).rglob("*")
                  if p.suffix in {".hpp", ".h", ".cpp"} and p.is_file())


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--clang-format", default="clang-format-21")
    args = parser.parse_args(argv)
    steps = [
        ("include rules", [PY, "scripts/check_includes.py"]),
        ("line limit", [PY, "scripts/check_line_limit.py"]),
        ("repo maps current", [PY, "scripts/gen_repo_map.py", "--check"]),
        ("checker self-tests", [PY, "-m", "pytest", "-q", "scripts/tests"]),
        ("clang-format", [args.clang_format, "--dry-run", "--Werror", *format_targets()]),
    ]
    failed = []
    for name, cmd in steps:
        proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
        status = "ok" if proc.returncode == 0 else f"FAILED (exit {proc.returncode})"
        print(f"[{status}] {name}")
        if proc.returncode != 0:
            failed.append(name)
            print("\n".join((proc.stdout + proc.stderr).splitlines()[:40]))
    print(f"check_all: {len(failed)} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
