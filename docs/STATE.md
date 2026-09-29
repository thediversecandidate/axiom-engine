# STATE

## Current
* **Stage:** 2 (Math), branch `stage-2-math` (create from `main`). Stages 0 and 1 merged (see `docs/HISTORY.md`).
* **Next atomic step:** 2a — `Vector3` (12 bytes) and `Vector4` (`alignas(16)`) with scalar operations, `tryNormalize` (empty for zero-length and non-finite input), and their tests (math; CONVENTIONS §3 + §4 CPU types).
* **Remaining Stage 2 steps (proposed):** 2b `Matrix4`; 2c `Quaternion` + known-answer rotations; 2d `Transform` + ordering tests; 2e accumulation tests; 2f AVX2 paths equal to scalar; 2g 10,000-case quaternion/matrix agreement.
* **Sizing rule (found in step 1):** new code per session must fit the working room (3,268 tokens ≈ 10 KB at the fallback rate), so steps are split to about one source file plus its test.

## Last modified
Stage 1 merged (PR #2); details in `docs/HISTORY.md`.

## Open items
* **Stale remote branches (delete in GitHub; this session gets 403):** `ci-probe-image-upload` (deliberate sabotage used to verify the CI image upload; never merge), `stage-0-tooling` (merged).
* **Local builds:** use the override with Clang 19 (`-DAXIOM_CLANG_VERSION=19 -DAXIOM_ALLOW_UNPINNED_COMPILER=ON`); Clang 18 cannot build `std::expected` with libstdc++ (ADR-0004).
1. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
2. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (Stage 0) | yes | n/a (hosted) | 0 (3 CI infra fixes) | n/a |
| 2026-09-28 | hosted Claude (Stage 1: 1a–7c) | yes | n/a (hosted) | 2 (committed with a failing check twice; commits are now gated on the full local run) | n/a |
