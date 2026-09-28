#!/usr/bin/env python3
"""Host-side context guard (CONTEXT_RULES §2): a proxy between the coding agent and the local model.

usage: ctx_guard.py --upstream http://127.0.0.1:11434 [--port 11435]

The agent is pointed at this proxy instead of the model server. For every chat request it counts
the fully serialized conversation with the pinned tokenizer and chat template:
  * occupied <= 23,268 tokens: forwarded unchanged (Ollama requests also get num_ctx = 32,768).
  * first request above 23,268: checkpoint-only mode. An instruction to update docs/STATE.md and
    stop is appended, and generation is capped at 1,500 tokens.
  * any later request: refused with HTTP 409 (AX-CTX-002); the session is over.
Supported request shapes: Ollama /api/chat, OpenAI /v1/chat/completions, Anthropic /v1/messages.
"""
from __future__ import annotations

import argparse
import json
import sys
import threading
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from axiom_tokens import load_counter  # noqa: E402

WINDOW, GENERATION_RESERVE, CHECKPOINT_RESERVE = 32768, 8000, 1500
THRESHOLD = WINDOW - GENERATION_RESERVE - CHECKPOINT_RESERVE  # 23,268
CHECKPOINT_TEXT = ("CHECKPOINT-ONLY MODE (AX-CTX-001): the context budget is exhausted. Your only allowed "
                   "action is to update docs/STATE.md (task, last modified files, open bugs, next atomic step, "
                   "session metrics). Then stop.")


def _text(content) -> str:
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        return "\n".join(_text(part.get("text", part.get("content", ""))) if isinstance(part, dict) else str(part)
                         for part in content)
    return "" if content is None else json.dumps(content)


def messages_of(body: dict) -> list[tuple[str, str]]:
    msgs = []
    if "system" in body:
        msgs.append(("system", _text(body["system"])))
    msgs += [(m.get("role", "user"), _text(m.get("content"))) for m in body.get("messages", [])]
    if "tools" in body:
        msgs.append(("system", json.dumps(body["tools"])))
    return msgs


class Guard:
    """Pure policy: decides what happens to each request. Thread-safe."""

    def __init__(self, counter=None, threshold: int = THRESHOLD):
        self.counter = counter or load_counter()
        self.threshold = threshold
        self.state = "normal"  # normal -> checkpoint -> closed
        self._lock = threading.Lock()

    def decide(self, body: dict, path: str) -> tuple[str, dict | None, int]:
        occupied = self.counter.count_messages(messages_of(body))
        with self._lock:
            if self.state == "closed" or (self.state == "checkpoint" and occupied > self.threshold):
                self.state = "closed"
                return "refuse", None, occupied
            if occupied <= self.threshold:
                return "forward", self._with_ctx(body, path), occupied
            self.state = "checkpoint"
            return "checkpoint", self._checkpoint(body, path), occupied

    @staticmethod
    def _with_ctx(body: dict, path: str) -> dict:
        body = dict(body)
        if path.startswith("/api/"):
            body["options"] = {**body.get("options", {}), "num_ctx": WINDOW}
        return body

    def _checkpoint(self, body: dict, path: str) -> dict:
        body = self._with_ctx(body, path)
        body["messages"] = list(body.get("messages", [])) + [{"role": "user", "content": CHECKPOINT_TEXT}]
        if path.startswith("/api/"):
            body["options"] = {**body["options"], "num_predict": CHECKPOINT_RESERVE}
        else:
            body["max_tokens"] = CHECKPOINT_RESERVE
        return body


def make_handler(guard: Guard, upstream: str):
    class Handler(BaseHTTPRequestHandler):
        def do_POST(self):  # noqa: N802
            raw = self.rfile.read(int(self.headers.get("Content-Length", 0)))
            try:
                body = json.loads(raw or b"{}")
            except json.JSONDecodeError:
                return self._reply(400, {"error": "AX-CTX-003 request body is not JSON"})
            action, new_body, occupied = guard.decide(body, self.path)
            if action == "refuse":
                return self._reply(409, {"error": "AX-CTX-002 session closed: context budget exhausted "
                                                  f"({occupied} tokens); start a new session"})
            data = json.dumps(new_body).encode()
            headers = {k: v for k, v in self.headers.items() if k.lower() not in {"host", "content-length"}}
            req = urllib.request.Request(upstream + self.path, data=data, headers=headers, method="POST")
            try:
                with urllib.request.urlopen(req) as resp:
                    payload, status = resp.read(), resp.status
                    ctype = resp.headers.get("Content-Type", "application/json")
            except urllib.error.HTTPError as err:
                payload, status, ctype = err.read(), err.code, "application/json"
            self.send_response(status)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(payload)))
            self.send_header("X-Axiom-Occupied-Tokens", str(occupied))
            self.end_headers()
            self.wfile.write(payload)

        def _reply(self, status: int, obj: dict):
            data = json.dumps(obj).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def log_message(self, *_):
            pass

    return Handler


def serve(upstream: str, port: int, guard: Guard | None = None) -> ThreadingHTTPServer:
    server = ThreadingHTTPServer(("127.0.0.1", port), make_handler(guard or Guard(), upstream.rstrip("/")))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--upstream", default="http://127.0.0.1:11434")
    parser.add_argument("--port", type=int, default=11435)
    args = parser.parse_args(argv)
    server = serve(args.upstream, args.port)
    print(f"ctx_guard: proxying 127.0.0.1:{args.port} -> {args.upstream} (threshold {THRESHOLD} tokens)")
    try:
        threading.Event().wait()
    except KeyboardInterrupt:
        server.shutdown()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
