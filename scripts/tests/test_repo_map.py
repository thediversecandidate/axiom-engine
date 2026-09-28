"""gen_repo_map.py self-tests: parse errors, budgets and staleness are all detected."""
import subprocess
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent.parent
GEN = str(SCRIPTS / "gen_repo_map.py")


def make_repo(tmp_path: Path, header: str) -> Path:
    inc = tmp_path / "modules" / "core" / "include" / "axiom" / "core"
    inc.mkdir(parents=True)
    (inc / "api.hpp").write_text(header)
    return tmp_path


def gen(root: Path, *extra: str):
    return subprocess.run([sys.executable, GEN, "--root", str(root), *extra], capture_output=True, text=True)


def test_contract_lines_are_extracted(tmp_path):
    root = make_repo(tmp_path, "#pragma once\n// Purpose line.\nnamespace axiom::core {\n"
                               "/// @owns the buffer\n/// @thread main only\nvoid f(int x);\n}\n")
    assert gen(root).returncode == 0
    text = (root / "docs" / "repomap" / "core.md").read_text()
    assert "`void f(int x)`" in text and "/// @owns the buffer" in text and "/// @thread main only" in text


def test_parse_error_is_reported(tmp_path):
    root = make_repo(tmp_path, "#pragma once\nnamespace axiom::core { void f( ; }\n")
    proc = gen(root)
    assert proc.returncode != 0 and "AX-MAP-001" in proc.stdout


def test_over_budget_is_reported_not_truncated(tmp_path):
    body = "".join(f"/// @errors returns empty on failure number {i}\nint function_number_{i}(int a, int b);\n"
                   for i in range(400))
    root = make_repo(tmp_path, "#pragma once\nnamespace axiom::core {\n" + body + "}\n")
    proc = gen(root)
    assert proc.returncode != 0 and "AX-MAP-002" in proc.stdout
    assert not (root / "docs" / "repomap" / "core.md").exists(), "over-budget maps must not be written"


def test_stale_map_is_reported(tmp_path):
    root = make_repo(tmp_path, "#pragma once\nnamespace axiom::core { void f(); }\n")
    assert gen(root).returncode == 0
    header = root / "modules" / "core" / "include" / "axiom" / "core" / "api.hpp"
    header.write_text("#pragma once\nnamespace axiom::core { void f(); void g(); }\n")
    proc = gen(root, "--check")
    assert proc.returncode != 0 and "AX-MAP-003" in proc.stdout
    before = (root / "docs" / "repomap" / "core.md").read_text()
    assert "g()" not in before, "--check must not modify committed maps"


def test_real_repository_maps_are_current():
    proc = gen(SCRIPTS.parent, "--check")
    assert proc.returncode == 0, proc.stdout
