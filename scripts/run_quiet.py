#!/usr/bin/env python3
"""Diagnostics wrapper (CONVENTIONS §1): full log to disk, bounded summary to the agent.

usage: run_quiet.py [--timeout SECONDS] [--log-dir DIR] -- <command...>

* The command runs in a new session (its own process group) with output written straight to a
  log file, so descendants holding stdout can never block this wrapper.
* Exit code: the child's own code; 128+N if killed by signal N; 124 on timeout.
* On timeout or after the child exits, the whole process group is terminated (SIGTERM, a bounded
  grace period, then SIGKILL) and reaped.
* Printed summary: at most 60 lines and 1,000 tokens; long lines truncated; full-log path appended.
"""
from __future__ import annotations

import argparse
import os
import re
import signal
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from axiom_tokens import load_counter  # noqa: E402

MAX_LINES = 60
MAX_TOKENS = 1000
MAX_LINE_CHARS = 300
GRACE_SECONDS = 2.0
ERROR_RE = re.compile(r"\berror:|\bError\b|\bFAILED\b|fatal error|Traceback|runtime error|Sanitizer:|CMake Error")


def _group_alive(pgid: int) -> bool:
    try:
        os.killpg(pgid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def terminate_group(pgid: int, grace: float = GRACE_SECONDS) -> None:
    """SIGTERM the group, wait up to `grace` seconds, then SIGKILL whatever is left."""
    for sig, wait in ((signal.SIGTERM, grace), (signal.SIGKILL, grace)):
        try:
            os.killpg(pgid, sig)
        except ProcessLookupError:
            return
        deadline = time.monotonic() + wait
        while time.monotonic() < deadline:
            if not _group_alive(pgid):
                return
            time.sleep(0.05)


def summarize(lines: list[str], log_path: Path, counter) -> str:
    lines = [ln if len(ln) <= MAX_LINE_CHARS else ln[:MAX_LINE_CHARS] + " …[truncated]" for ln in lines]
    first_error = next((i for i, ln in enumerate(lines) if ERROR_RE.search(ln)), None)
    tail_start = max(0, len(lines) - (MAX_LINES - 1))
    if first_error is None or first_error >= tail_start:
        picked = lines[tail_start:]
    else:  # first error block, then the tail, without repeating lines
        block_end = min(first_error + 20, len(lines))
        tail_len = max(0, MAX_LINES - 2 - (block_end - first_error))
        tail_from = max(block_end, len(lines) - tail_len)
        picked = lines[first_error:block_end] + (["…"] if tail_from > block_end else []) + lines[tail_from:]
    footer = f"[full log: {log_path}]"
    out: list[str] = []
    for ln in picked[: MAX_LINES - 1]:
        if counter.count("\n".join(out + [ln, footer])) > MAX_TOKENS:
            break
        out.append(ln)
    out.append(footer)
    return "\n".join(out)


def run(command: list[str], timeout: float | None, log_dir: Path) -> tuple[int, Path]:
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / f"run_{int(time.time() * 1000)}_{os.getpid()}.log"
    with open(log_path, "wb") as log:
        proc = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                start_new_session=True)
        pgid = proc.pid
        try:
            code = proc.wait(timeout=timeout)
            code = code if code >= 0 else 128 + (-code)
        except subprocess.TimeoutExpired:
            terminate_group(pgid)
            proc.wait()
            log.write(f"\n[run_quiet] TIMEOUT after {timeout}s; process group killed\n".encode())
            code = 124
        terminate_group(pgid)  # reap any descendants still alive after the child exited
    return code, log_path


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--timeout", type=float, default=None)
    parser.add_argument("--log-dir", type=Path, default=Path("build/logs"))
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("no command given")
    code, log_path = run(command, args.timeout, args.log_dir)
    text = log_path.read_text(errors="replace").splitlines()
    print(summarize(text, log_path, load_counter()))
    print(f"[exit {code}]")
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
