"""expect_failure.py self-tests: only a real, matching sanitizer-style failure passes."""
import sys

from expect_failure import check

PY = sys.executable


def test_plain_return_one_does_not_pass():
    ok, message = check([PY, "-c", "import sys; sys.exit(1)"], "runtime error", 30)
    assert not ok and message.startswith("AX-EXP-002")


def test_success_does_not_pass():
    ok, message = check([PY, "-c", "print('runtime error: fake')"], "runtime error", 30)
    assert not ok and message.startswith("AX-EXP-001")


def test_matching_failure_passes():
    code = "import sys; print('x.cpp:3: runtime error: signed integer overflow'); sys.exit(1)"
    ok, _ = check([PY, "-c", code], "runtime error: signed integer overflow", 30)
    assert ok


def test_timeout_does_not_pass():
    ok, message = check([PY, "-c", "import time; time.sleep(5)"], "x", 0.5)
    assert not ok and message.startswith("AX-EXP-003")
