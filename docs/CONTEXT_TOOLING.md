# Axiom Engine — Context Tooling & Workflow (loaded for infra and Doom-fork tasks)

## 1. Enforcement Tools
* `scripts/session_manifest.py`: counts the serialized start-of-session request per slot (CONTEXT_RULES §2); refuses over-budget loads (AX-SES-001..003). Context files are complete or refused, never truncated.
* `scripts/run_quiet.py`: diagnostics only. Full log to `build/logs/`; summary ≤ 60 lines and ≤ 1,000 tokens; child exit code preserved; signals → 128+N; timeout → 124 after terminating and killing the whole process group.
* `scripts/ctx_guard.py`: the host-side proxy between agent and local model. It forwards requests at or below 23,268 occupied tokens, switches to checkpoint-only mode on the first request above it (STATE.md update, 1,500-token generation cap), and refuses later requests with HTTP 409 (AX-CTX-002).
* `scripts/axiom_tokens.py`: shared counter. Uses the pinned tokenizer when present, otherwise the conservative ceil(bytes/3) fallback, labelled "fallback" (ADR-0003).

## 2. Repository Maps (`scripts/gen_repo_map.py`)
* Per public declaration: signature plus `/// @owns @lifetime @thread @errors` lines; per module: headers with one-line purpose. Boilerplate special members (default/copy/move constructors, assignments, destructor) collapse into one trait line such as `(move-only, default-constructible)`; any other constructor is listed.
* Never truncated. Parse error → AX-MAP-001; over budget (index 1,000, module 2,500) → AX-MAP-002; `--check` regenerates into a temp directory and diffs → AX-MAP-003.

## 3. File Limits (`scripts/check_line_limit.py`)
≤ 300 physical lines for `.hpp .h .cpp .glsl .vert .frag .comp .py`, excluding `build/`, `_deps/`, `scripts/tests/fixtures/` (AX-LEN-001). Split by responsibility.

## 4. Checker Self-Tests
Fixtures in `scripts/tests/fixtures/<checker>/{pass,fail}/<case>/`, each run in isolation: `pass` cases exit 0; `fail` cases exit non-zero AND print the code in `expected.txt`. Include-checker cases cover direct, relative, private-`src/` and transitive includes.

## 5. Workflow
* One agent per session through Stage 3; at the end of Stage 3 an ADR decides on orchestration using STATE.md metrics.
* Branch per stage; commit only after the Definition of Done; merge when the stage's acceptance tests pass in CI.
* Reference images change only in a commit that states why and was viewed by the developer.
* Repositories: `axiom-engine` (MIT) and `axiom-doom3` (GPL-3.0). No code moves from `axiom-doom3` into `axiom-engine`.

## 6. Doom Fork Legacy-Code Policy
* `axiom-doom3` has its own build and checker manifest. Inherited upstream files keep their layout and are explicit line-limit exceptions (e.g. `AI.cpp` is 6,165 lines at v1.6.0).
* Inherited files over context limits are loaded as complete functions plus required declarations, types and globals, never partial functions. Each entry pins the translation-unit hash, the exact `compile_commands.json` directory and argv (macros, forced includes, language mode, include paths) and the enclosing conditional state, and hashes the preprocessed function tokens. A context that does not fit rejects the task. Revalidate before patching; rebuild the whole translation unit after.
* New files (adapters, learned-policy hooks) follow all Axiom rules.
