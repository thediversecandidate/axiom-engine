# STATE

## Current
* **Stage:** 2 (Math), branch `stage-2-math`. Stages 0 and 1 merged (see `docs/HISTORY.md`).
* **Done in Stage 2:** 2a `Vector3`/`Vector4` (see `docs/HISTORY.md`).
* **Next atomic step:** 2b — `Matrix4` (column-major `float data[16]`, `alignas(16)`, `v' = M·v`): identity, multiply, transform point/direction, transpose, determinant, inverse returning `std::optional` (empty when singular or non-finite), with tests (math; CONVENTIONS §3 + §4).
* **Remaining Stage 2 steps (proposed):** 2c `Quaternion` + known-answer rotations; 2d `Transform` + ordering tests; 2e accumulation tests; 2f AVX2 paths equal to scalar; 2g 10,000-case quaternion/matrix agreement.
* **Sizing rule (found in step 1):** new code per session must fit the working room (3,268 tokens ≈ 10 KB at the fallback rate), so steps are split to about one source file plus its test.

## Last modified
`modules/math`: `vector.hpp`, `src/vector.cpp`, private `src/math_assert.hpp` (`AXIOM_MATH_ASSERT`), `tests/test_vector.cpp`, assert fixture.

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
| 2026-09-29 | hosted Claude (Stage 2: 2a) | yes | n/a (hosted) | 0 | n/a |
