# STATE

## Current
* **Stage:** 1 (Vertical Slice), branch `stage-1-triangle`. Stage 0 merged (see `docs/HISTORY.md`).
* **Done in Stage 1:** steps 1a–5 (instance, device, allocator, offscreen target, shaders, triangle pipeline, landmark/coverage and culling tests); per-step detail in `docs/HISTORY.md`.
* **Next atomic step:** 6 — full-image comparison to a reviewed reference (≤ 65 px differ by > 2/255), actual/reference/diff images uploaded as CI artifacts on failure (renderer + CI workflow; reference generated once by a script and reviewed, never regenerated automatically).
* **Remaining Stage 1 steps:** 7 SDL3 swapchain path (local only); then PR and merge.
* **Sizing rule (found in step 1):** new code per session must fit the working room (3,268 tokens ≈ 10 KB at the fallback rate), so steps are split to about one source file plus its test.

## Last modified
`modules/renderer/tests/` (test_triangle_culling.cpp; scene loading moved into triangle_fixture.hpp); renderer CMake.

## Open items
* **Local builds:** use the override with Clang 19 (`-DAXIOM_CLANG_VERSION=19 -DAXIOM_ALLOW_UNPINNED_COMPILER=ON`); Clang 18 cannot build `std::expected` with libstdc++ (ADR-0004).
1. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
2. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (Stage 0) | yes | n/a (hosted) | 0 (3 CI infra fixes) | n/a |
| 2026-09-28 | hosted Claude (Stage 1: 1a–5) | yes | n/a (hosted) | 2 (committed with a failing check twice; commits are now gated on the full local run) | n/a |
