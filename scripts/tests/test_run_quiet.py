"""run_quiet.py self-tests (CONVENTIONS §1, ROADMAP Stage 0)."""
import os
import subprocess
import sys
import time
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent.parent
RUN_QUIET = str(SCRIPTS / "run_quiet.py")
PY = sys.executable


def run(tmp_path, code: str, timeout: float | None = None):
    cmd = [PY, RUN_QUIET, "--log-dir", str(tmp_path)]
    if timeout is not None:
        cmd += ["--timeout", str(timeout)]
    cmd += ["--", PY, "-c", code]
    start = time.monotonic()
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    return proc, time.monotonic() - start


def test_success_returns_zero(tmp_path):
    proc, _ = run(tmp_path, "print('hello')")
    assert proc.returncode == 0
    assert "hello" in proc.stdout


def test_exit_code_is_preserved(tmp_path):
    proc, _ = run(tmp_path, "import sys; sys.exit(7)")
    assert proc.returncode == 7


def test_signal_termination_is_nonzero(tmp_path):
    proc, _ = run(tmp_path, "import os, signal; os.kill(os.getpid(), signal.SIGKILL)")
    assert proc.returncode == 128 + 9


def test_timeout_kills_grandchild_holding_stdout(tmp_path):
    marker = tmp_path / "grandchild.pid"
    code = ("import subprocess, sys, time\n"
            "gc = subprocess.Popen([sys.executable, '-c', 'import signal, time; "
            "signal.signal(signal.SIGTERM, signal.SIG_IGN); time.sleep(60)'])\n"
            f"open({str(marker)!r}, 'w').write(str(gc.pid))\n"
            "time.sleep(60)\n")
    proc, elapsed = run(tmp_path, code, timeout=1.0)
    assert proc.returncode == 124
    assert elapsed < 15, "wrapper must return by the deadline plus grace periods"
    pid = int(marker.read_text())
    try:
        os.kill(pid, 0)
        alive = Path(f"/proc/{pid}/status").read_text().find("State:\tZ") == -1
    except ProcessLookupError:
        alive = False
    assert not alive, "grandchild ignoring SIGTERM must be killed"


def test_failure_beyond_output_cap(tmp_path):
    code = "import sys\nfor i in range(500): print('line', i)\nprint('error: late failure')\nsys.exit(3)"
    proc, _ = run(tmp_path, code)
    assert proc.returncode == 3
    printed = proc.stdout.splitlines()
    assert len(printed) <= 62  # 60-line summary + footer + exit line
    log = next(tmp_path.glob("run_*.log")).read_text()
    assert "line 0\n" in log and "line 499" in log and "error: late failure" in log


def test_long_lines_are_truncated_in_summary(tmp_path):
    proc, _ = run(tmp_path, "print('x' * 5000)")
    assert proc.returncode == 0
    assert max(len(line) for line in proc.stdout.splitlines()) < 400
