# Axiom Engine — LLM Context Rules (loaded every session)

Target: a local model with a 32,768-token window. Counts use the pinned tokenizer and chat template in `scripts/tokenizer/` on the fully serialized request (ADR-0003 for the fallback). Tool details: `docs/CONTEXT_TOOLING.md`.

## 1. Modules
* `modules/<name>/include/axiom/<name>/*.hpp` public, `src/` private, `tests/`.
* Allowed: `core` → none; `math` → none; `physics` → core, math; `renderer_vk` → core, math; `renderer` → core, math, renderer_vk (never physics); `ai` → core, math; `app` → all. Never include another module's `src/`.

## 2. Session Budget (tokens)
| Slot | Max |
| --- | --- |
| Generation reserve (never loaded) | 8,000 |
| Agent system prompt + tools (measured, in STATE.md) | 3,500 |
| `CLAUDE.md` + `STATE.md` + this file + `CONVENTIONS.md` §2 | 3,000 |
| Required sections (§3) + current ROADMAP stage | 3,500 |
| `repomap/index.md` + target module map | 3,000 |
| Dependency public headers | 3,000 |
| Target files | 4,000 |
| Checkpoint reserve | 1,500 |
| Working room (turns + tool output) | 3,268 |
| **Total** | **32,768** |
* Run `scripts/session_manifest.py` first; files load complete or not at all. Diagnostics go through `scripts/run_quiet.py` (≤ 60 lines, ≤ 1,000 tokens).
* Above 23,268 occupied tokens the host allows only a STATE.md update, then ends the session.

## 3. What to Load
1. `CLAUDE.md`, `docs/STATE.md`, this file, `docs/CONVENTIONS.md` §2.
2. The union of sections required by the task's responsibilities and the stage's acceptance rules: build/CI → CONVENTIONS §1 + `CONTEXT_TOOLING.md`; math → §3; physics or core time/simulation → §3 + §5; rendering/shaders (`renderer`, `renderer_vk`) → §3 + §4; ai → §3 + §5 + §6; app → §3–§6; evaluation/benchmarks → §6; assets, licensing, voices → §7; Doom fork → `CONTEXT_TOOLING.md` §6. Plus the current ROADMAP stage.
3. Story/art authoring loads all of `CREATIVE_DIRECTION.md` + §7. Code implementing a creative test loads its CREATIVE_DIRECTION §8 row and every section it references, in addition to the union above.
4. `docs/repomap/index.md` and the target module's map; target files in full; dependencies as public headers only.
* If the union does not fit, split the task; never drop a required section. One atomic step in one module per session (`infra` tasks may span directories).

## 4. State & Decisions
* `docs/STATE.md`: stage, task (`infra` flag), last modified files, open bugs, next atomic step, measured system-prompt tokens, per-session metrics (completed, peak context, DoD failures, peak VRAM). Read at start, update at end.
* `docs/adr/ADR-NNNN-title.md` for every trade-off or rule change.

## 5. Definition of Done
1. `debug`, `asan-ubsan`, `release` build; all tests pass in all three.
2. `python3 scripts/check_all.py` passes (include rules, line limit, repo maps current, checker self-tests, clang-format).
3. STATE.md updated; ADR written if a rule changed.
