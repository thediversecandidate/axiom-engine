"""Checker self-tests (CONTEXT_TOOLING §4): every fixture case runs in isolation.

pass/ cases must exit 0; fail/ cases must exit non-zero AND print the code in expected.txt.
A checker that always fails, or always passes, cannot satisfy both halves.
"""
import subprocess
import sys
from pathlib import Path

import pytest

SCRIPTS = Path(__file__).resolve().parent.parent
FIXTURES = Path(__file__).resolve().parent / "fixtures"
CHECKERS = {"check_includes": "check_includes.py", "check_line_limit": "check_line_limit.py"}


def cases():
    for checker in CHECKERS:
        for kind in ("pass", "fail"):
            for case in sorted((FIXTURES / checker / kind).iterdir()):
                yield pytest.param(checker, kind, case, id=f"{checker}-{kind}-{case.name}")


@pytest.mark.parametrize("checker,kind,case", list(cases()))
def test_fixture(checker, kind, case):
    proc = subprocess.run([sys.executable, str(SCRIPTS / CHECKERS[checker]), "--root", str(case)],
                          capture_output=True, text=True)
    expected = (case / "expected.txt").read_text().split()
    if kind == "pass":
        assert proc.returncode == 0, proc.stdout
    else:
        assert proc.returncode != 0, proc.stdout
        for code in expected:
            assert code in proc.stdout, f"expected {code} in:\n{proc.stdout}"


def test_every_include_rule_has_a_fail_case():
    codes = {c for p in (FIXTURES / "check_includes" / "fail").glob("*/expected.txt") for c in p.read_text().split()}
    assert codes == {"AX-INC-001", "AX-INC-002", "AX-INC-003", "AX-INC-004"}


def test_real_repository_is_clean():
    root = SCRIPTS.parent
    for script in CHECKERS.values():
        proc = subprocess.run([sys.executable, str(SCRIPTS / script), "--root", str(root)],
                              capture_output=True, text=True)
        assert proc.returncode == 0, proc.stdout
