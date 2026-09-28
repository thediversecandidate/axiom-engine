"""Launcher tests (ROADMAP Stage 0): session_manifest.py and ctx_guard.py."""
import json
import threading
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

import ctx_guard
import session_manifest
from axiom_tokens import load_counter

# ---------- session_manifest: complete-or-refused loading, serialization overhead ----------


def repo(tmp_path: Path, target_bytes: int) -> Path:
    (tmp_path / "docs").mkdir()
    (tmp_path / "docs" / "CONVENTIONS.md").write_text("# C\n## 2. C++ Rules\nrule\n## 3. Math\nmath rule\n")
    src = tmp_path / "modules" / "core" / "src"
    src.mkdir(parents=True)
    (src / "a.cpp").write_text("x" * target_bytes)
    return tmp_path


def test_small_task_is_admitted(tmp_path):
    root = repo(tmp_path, 300)
    usage, total, _, problems = session_manifest.plan(
        root, ["docs/CONVENTIONS.md#3", "modules/core/src/a.cpp"], "core", 1000)
    assert problems == []
    assert usage["sections"] > 0 and usage["target_files"] > 0 and total > 1000


def test_oversized_file_is_refused_never_truncated(tmp_path):
    root = repo(tmp_path, 60_000)  # far beyond the 4,000-token target-files slot
    _, _, _, problems = session_manifest.plan(root, ["modules/core/src/a.cpp"], "core", 1000)
    assert any(p.startswith("AX-SES-001") and "target_files" in p for p in problems)


def test_missing_section_is_reported(tmp_path):
    root = repo(tmp_path, 10)
    _, _, _, problems = session_manifest.plan(root, ["docs/CONVENTIONS.md#9"], "core", 0)
    assert any(p.startswith("AX-SES-002") for p in problems)


def test_serialization_overhead_is_counted():
    counter = load_counter()
    content = "hello world " * 50
    raw = counter.count(content)
    serialized = counter.count_messages([("user", content)])
    assert serialized > raw, "chat-template tokens must be included in the count"


# ---------- ctx_guard: forward, checkpoint-only transition, refusal ----------


class FixedCounter:
    method = "test"

    def __init__(self):
        self.next = 0

    def count_messages(self, _messages):
        return self.next


def test_guard_policy_transitions():
    counter = FixedCounter()
    guard = ctx_guard.Guard(counter=counter, threshold=100)
    counter.next = 50
    action, body, _ = guard.decide({"messages": []}, "/api/chat")
    assert action == "forward" and body["options"]["num_ctx"] == 32768
    counter.next = 150
    action, body, _ = guard.decide({"messages": []}, "/api/chat")
    assert action == "checkpoint"
    assert body["options"]["num_predict"] == 1500 and "CHECKPOINT-ONLY" in body["messages"][-1]["content"]
    action, _, _ = guard.decide({"messages": []}, "/api/chat")
    assert action == "refuse"


def test_threshold_matches_budget():
    assert ctx_guard.THRESHOLD == 32768 - 8000 - 1500 == 23268


def test_proxy_end_to_end(tmp_path):
    received = []

    class Upstream(BaseHTTPRequestHandler):
        def do_POST(self):  # noqa: N802
            received.append(json.loads(self.rfile.read(int(self.headers["Content-Length"]))))
            data = b'{"message": {"role": "assistant", "content": "ok"}}'
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def log_message(self, *_):
            pass

    upstream = ThreadingHTTPServer(("127.0.0.1", 0), Upstream)
    threading.Thread(target=upstream.serve_forever, daemon=True).start()
    guard = ctx_guard.Guard(threshold=40)
    proxy = ctx_guard.serve(f"http://127.0.0.1:{upstream.server_port}", 0, guard)
    url = f"http://127.0.0.1:{proxy.server_port}/api/chat"

    def post(text):
        req = urllib.request.Request(url, data=json.dumps({"messages": [{"role": "user", "content": text}]}).encode(),
                                     headers={"Content-Type": "application/json"}, method="POST")
        return urllib.request.urlopen(req)

    try:
        assert post("short").status == 200
        assert post("long " * 200).status == 200  # checkpoint-only request still forwarded
        assert "CHECKPOINT-ONLY" in received[-1]["messages"][-1]["content"]
        try:
            post("long " * 200)
            raise AssertionError("expected HTTP 409 after checkpoint")
        except urllib.error.HTTPError as err:
            assert err.code == 409 and b"AX-CTX-002" in err.read()
    finally:
        proxy.shutdown()
        upstream.shutdown()
