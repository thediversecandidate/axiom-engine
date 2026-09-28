#!/usr/bin/env python3
"""Sanitizer detection harness (CONVENTIONS §1).

usage: expect_failure.py --pattern REGEX [--timeout S] -- <command...>

Passes (exit 0) only if the command exits non-zero AND its complete, untruncated output matches
REGEX. A command that fails for any other reason (e.g. a plain `return 1`) does not pass.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys


def check(command: list[str], pattern: str, timeout: float) -> tuple[bool, str]:
    try:
        proc = subprocess.run(command, capture_output=True, timeout=timeout, text=True, errors="replace")
    except subprocess.TimeoutExpired:
        return False, "AX-EXP-003 command timed out"
    output = proc.stdout + proc.stderr
    if proc.returncode == 0:
        return False, "AX-EXP-001 command exited 0; expected a sanitizer failure"
    if not re.search(pattern, output):
        return False, f"AX-EXP-002 exit {proc.returncode} but output does not match /{pattern}/"
    return True, f"ok: exit {proc.returncode}, report matched /{pattern}/"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pattern", required=True)
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    ok, message = check(command, args.pattern, args.timeout)
    print(message)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
