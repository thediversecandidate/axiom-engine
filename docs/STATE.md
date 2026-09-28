# STATE

## Current
* **Stage:** 0 (Ground Truth & Tooling), branch `stage-0-tooling`
* **Task:** Stage 0 scaffold (`infra`)
* **Next atomic step:** merge PR #1 once CI is green on the digest-pinned image; then Stage 1, step 1: Vulkan instance + device creation with explicit feature enables and the debug messenger (renderer module).

## Last modified
Entire repository (initial scaffold): CMake/presets/toolchain, 6 module skeletons, tests, sanitizer fixtures, scripts + self-tests, CI image and workflow, docs.

## Verified locally (Clang 18 via the local-only override; not a Definition-of-Done result)
* `debug`, `asan-ubsan`, `release` build; all tests pass (7 / 11 / 7).
* UBSan and LeakSanitizer fixtures fail with their expected reports; clean controls pass.
* 33 script self-tests pass, including the timeout test with a SIGTERM-ignoring grandchild.
* `check_all.py` clean; lavapipe check passes with validation layers installed.

## Verified in CI (Definition of Done, PR #1)
* CI image: Ubuntu 26.04 (digest-pinned), apt snapshot 20260927T000000Z; Clang 21.1.8, CMake 4.2.3, Python 3.14.4.
* `debug`, `asan-ubsan`, `release`: first configure with an empty dependency cache, build and all tests pass on Clang 21 + libc++ 21.
* `check_all.py` (includes, line limit, repo maps, 33 self-tests, clang-format 21) passes; lavapipe selected, llvmpipe reported, validation layer present.

## Found during Stage 0
* First CI run: the HTTPS-only snapshot service needs `ca-certificates` bootstrapped into the bare base image (ADR-0002).
* CI logs are unreachable from the authoring environment, so every CI step reports failures and key facts as annotations (`scripts/ci_annotate.py`).
* The first real pre-flight refused an ordinary session: the always-loaded docs were 3,735 of 3,000 tokens under the fallback counter. Fixed by splitting `CONTEXT_RULES.md` (ADR-0003); now 2,409 of 3,000.

## Open items
1. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
2. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (scaffold) | yes | n/a (hosted) | 0 (3 CI infra fixes) | n/a |
