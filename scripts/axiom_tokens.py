"""Token counting shared by all Axiom context tools (CONTEXT_RULES §2).

Counts use the pinned local-model tokenizer in scripts/tokenizer/ when present:
  tokenizer.json        Hugging Face tokenizers file of the local model
  chat_template.txt     per-message template with {role} and {content} placeholders
  tokenizer.lock        "<sha256>  <filename>" lines for both files
Until those are pinned, counts fall back to ceil(bytes / 3), which over-estimates typical
English and code (about 3.5–4 bytes per token), and every result is labelled "fallback".
"""
from __future__ import annotations

import hashlib
import math
from dataclasses import dataclass
from pathlib import Path

TOKENIZER_DIR = Path(__file__).resolve().parent / "tokenizer"
DEFAULT_TEMPLATE = "<|im_start|>{role}\n{content}<|im_end|>\n"  # ChatML, used until a template is pinned


@dataclass(frozen=True)
class Counter:
    method: str  # "pinned" or "fallback"
    _tokenizer: object | None
    template: str

    def count(self, text: str) -> int:
        if self._tokenizer is not None:
            return len(self._tokenizer.encode(text, add_special_tokens=False).ids)
        return math.ceil(len(text.encode("utf-8")) / 3)

    def serialize(self, messages: list[tuple[str, str]]) -> str:
        """Serializes (role, content) pairs exactly as the chat template would."""
        return "".join(self.template.format(role=role, content=content) for role, content in messages)

    def count_messages(self, messages: list[tuple[str, str]]) -> int:
        return self.count(self.serialize(messages))


def _verify_lock(directory: Path) -> None:
    lock = directory / "tokenizer.lock"
    for line in lock.read_text().splitlines():
        if not line.strip():
            continue
        digest, name = line.split(maxsplit=1)
        actual = hashlib.sha256((directory / name.strip()).read_bytes()).hexdigest()
        if actual != digest:
            raise RuntimeError(f"AX-TOK-001 tokenizer file {name} does not match tokenizer.lock")


def load_counter(directory: Path = TOKENIZER_DIR) -> Counter:
    tokenizer_file = directory / "tokenizer.json"
    if not tokenizer_file.exists():
        return Counter("fallback", None, DEFAULT_TEMPLATE)
    _verify_lock(directory)
    from tokenizers import Tokenizer  # pinned in scripts/requirements.txt

    template_file = directory / "chat_template.txt"
    template = template_file.read_text() if template_file.exists() else DEFAULT_TEMPLATE
    return Counter("pinned", Tokenizer.from_file(str(tokenizer_file)), template)
