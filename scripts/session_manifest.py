#!/usr/bin/env python3
"""Session pre-flight (CONTEXT_RULES §2–3): counts the serialized start-of-session request.

usage: session_manifest.py --target-module M [--system-prompt-tokens N] <spec>...

A spec is a repo-relative path, optionally with a section: docs/CONVENTIONS.md#3,
docs/CREATIVE_DIRECTION.md#8, docs/ROADMAP.md#Stage 0. Files are loaded complete or not at all:
this tool never truncates. Diagnostics:
  AX-SES-001  a slot exceeds its budget
  AX-SES-002  a file or section does not exist
  AX-SES-003  the serialized start-of-session request exceeds the loadable total
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from axiom_tokens import load_counter  # noqa: E402

WINDOW, GENERATION_RESERVE, CHECKPOINT_RESERVE, WORKING_ROOM = 32768, 8000, 1500, 3268
SLOT_BUDGETS = {"system": 3500, "protocol": 3000, "sections": 3500, "maps": 3000, "dependency_headers": 3000,
                "target_files": 4000}
LOADABLE_TOTAL = WINDOW - GENERATION_RESERVE - CHECKPOINT_RESERVE - WORKING_ROOM  # 20,000
PROTOCOL = {"CLAUDE.md", "docs/STATE.md", "docs/CONTEXT_RULES.md", "docs/CONVENTIONS.md#2"}


def extract_section(text: str, section: str) -> str | None:
    """Returns the '## <section>...' block of a markdown file, or None."""
    blocks = re.split(r"(?m)^(?=## )", text)
    for block in blocks:
        heading = block.splitlines()[0] if block else ""
        name = heading[3:].strip()
        if name.startswith(f"{section}.") or name.startswith(f"{section}:") or name == section \
                or name.startswith(f"{section} "):
            return block
    return None


def load(root: Path, spec: str) -> str | None:
    path_part, _, section = spec.partition("#")
    path = root / path_part
    if not path.is_file():
        return None
    text = path.read_text(errors="replace")
    return extract_section(text, section) if section else text


def slot_of(spec: str, target_module: str) -> str:
    path = spec.partition("#")[0]
    if spec in PROTOCOL:
        return "protocol"
    if path in {"docs/CONVENTIONS.md", "docs/CREATIVE_DIRECTION.md", "docs/ROADMAP.md",
                "docs/CONTEXT_TOOLING.md"}:
        return "sections"
    if path.startswith("docs/repomap/"):
        return "maps"
    match = re.match(r"modules/([^/]+)/include/", path)
    if match and match.group(1) != target_module:
        return "dependency_headers"
    return "target_files"


def plan(root: Path, specs: list[str], target_module: str, system_tokens: int):
    counter = load_counter()
    usage = {slot: 0 for slot in SLOT_BUDGETS}
    usage["system"] = system_tokens
    messages, problems = [], []
    for spec in specs:
        content = load(root, spec)
        if content is None:
            problems.append(f"AX-SES-002 {spec}: file or section not found")
            continue
        message = ("user", f"=== {spec} ===\n{content}")
        messages.append(message)
        usage[slot_of(spec, target_module)] += counter.count_messages([message])
    for slot, used in usage.items():
        if used > SLOT_BUDGETS[slot]:
            problems.append(f"AX-SES-001 slot '{slot}': {used} tokens exceeds budget {SLOT_BUDGETS[slot]}")
    total = system_tokens + counter.count_messages(messages)
    if total > LOADABLE_TOTAL:
        problems.append(f"AX-SES-003 serialized request {total} tokens exceeds loadable total {LOADABLE_TOTAL}")
    return usage, total, counter.method, problems


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--target-module", required=True)
    parser.add_argument("--system-prompt-tokens", type=int, default=SLOT_BUDGETS["system"])
    parser.add_argument("specs", nargs="+")
    args = parser.parse_args(argv)
    usage, total, method, problems = plan(args.root, args.specs, args.target_module, args.system_prompt_tokens)
    for slot, used in usage.items():
        print(f"{slot:<20} {used:>6} / {SLOT_BUDGETS[slot]}")
    print(f"{'serialized total':<20} {total:>6} / {LOADABLE_TOTAL}   (counter: {method})")
    for p in problems:
        print(p)
    if problems:
        print("session_manifest: REFUSED — split the task; files are never truncated")
        return 1
    print("session_manifest: admitted")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
