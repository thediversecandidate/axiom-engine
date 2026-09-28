# Axiom Engine — Session Protocol (any coding agent)

1. Read `docs/STATE.md`, `docs/CONTEXT_RULES.md`, `docs/CONVENTIONS.md` §2.
2. Read every CONVENTIONS section the task's APIs require (CONTEXT_RULES §3), the current ROADMAP stage, and any CREATIVE_DIRECTION sections CONTEXT_RULES §3 requires.
3. Read `docs/repomap/index.md` and the target module's map.
4. Run `python3 scripts/session_manifest.py <files>`; if it refuses, split the task.
5. Load only those files; dependencies as public headers only.
6. Do the "next atomic step" from STATE.md, in one module (or one `infra` task).
7. Run every build/test command through `scripts/run_quiet.py`.
8. The host enforces the budget: above 23,268 occupied tokens only a STATE.md update is allowed, then the session ends.
9. Definition of Done (CONTEXT_RULES §5), then update STATE.md (including session metrics) and write an ADR if a convention changed.

Never: call `.value()` on `std::expected`; add exceptions/RTTI to engine code; let an exception cross an engine frame; make `math` depend on `core`; include `physics` from `renderer`; use a negative viewport height; skip `vkCmdSetFrontFace` on a draw; edit `_deps/`; regenerate reference images automatically; exceed 300 lines in a new file (inherited Doom fork files follow CONTEXT_TOOLING §6); commit Doom 3-derived content or clone a real voice; change CONVENTIONS.md without an ADR.
