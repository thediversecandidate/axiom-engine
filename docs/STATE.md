# STATE

## Current
* **Stage:** 0 (Ground Truth & Tooling), branch `stage-0-tooling`
* **Task:** Stage 0 scaffold (`infra`)
* **Next atomic step:** get CI green on the `stage-0-tooling` pull request. Then pin the `ubuntu:26.04` base image digest (ADR-0002 follow-up) and merge.

## Last modified
Entire repository (initial scaffold): CMake/presets/toolchain, 6 module skeletons, tests, sanitizer fixtures, scripts + self-tests, CI image and workflow, docs.

## Verified locally (Clang 18 via the local-only override; not a Definition-of-Done result)
* `debug`, `asan-ubsan`, `release` build; all tests pass (7 / 11 / 7).
* UBSan and LeakSanitizer fixtures fail with their expected reports; clean controls pass.
* 33 script self-tests pass, including the timeout test with a SIGTERM-ignoring grandchild.
* `check_all.py` clean; lavapipe check passes with validation layers installed.

## Found during Stage 0
* The first real pre-flight refused an ordinary session: the always-loaded docs were 3,735 of 3,000 tokens under the fallback counter. Fixed by splitting `CONTEXT_RULES.md` (ADR-0003); now 2,409 of 3,000.

## Open items
1. **Clang 21 build:** only CI can verify it (not installable in the authoring environment).
2. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
3. **Base image digest:** not yet pinned (ADR-0002).
4. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (scaffold) | yes, pending CI | n/a (hosted) | 0 local | n/a |
