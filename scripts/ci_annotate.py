#!/usr/bin/env python3
"""CI helper: runs a command, tees its output to a log, and on failure emits one GitHub error
annotation holding the last 50 log lines, so the failure is readable through the API.

usage: ci_annotate.py <title> <log-file> -- <command...>
"""
from __future__ import annotations

import subprocess
import sys


def encode(text: str) -> str:
    return text.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def main(argv: list[str]) -> int:
    title, log_path, rest = argv[0], argv[1], argv[2:]
    command = rest[1:] if rest[:1] == ["--"] else rest
    with open(log_path, "w") as log:
        proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                                errors="replace")
        assert proc.stdout is not None
        for line in proc.stdout:
            sys.stdout.write(line)
            log.write(line)
        code = proc.wait()
    if code != 0:
        with open(log_path, errors="replace") as log:
            tail = "".join(log.readlines()[-50:])
        print(f"::error title={title} (exit {code})::{encode(tail)}")
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
