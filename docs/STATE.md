# STATE

## Current
* **Stage:** 1 (Vertical Slice), branch `stage-1-triangle`. Stage 0 merged (see `docs/HISTORY.md`).
* **Done in Stage 1:** steps 1a–7a (instance, device, allocator, offscreen target, shaders, triangle pipeline, landmark/coverage, culling and reference-image tests, instance extensions + device presentation); per-step detail in `docs/HISTORY.md`.
* **Next atomic step:** 7b — `Swapchain` in renderer_vk (create for a surface + extent, acquire, present, recreate on OUT_OF_DATE), tested on lavapipe with VK_EXT_headless_surface: render the golden triangle into a swapchain image and present. renderer_vk map is 1,848 / 2,500 tokens; if the swapchain pushes it over, split presentation into its own module (ADR).
* **Remaining Stage 1 steps:** 7c SDL3 window app (local only; CI builds it); PR and merge; Derrick's visual sign-off on `tests/golden/triangle.reference.png`.
* **Sizing rule (found in step 1):** new code per session must fit the working room (3,268 tokens ≈ 10 KB at the fallback rate), so steps are split to about one source file plus its test.

## Last modified
`renderer_vk`: `InstanceDesc::extraExtensions`, `DeviceDesc::presentSurface` + `presentEnabled()`, `tests/test_presentation.cpp`; call sites now initialize every desc field (`-Wmissing-field-initializers`).

## Open items
* **Stale remote branches (delete in GitHub; this session gets 403):** `ci-probe-image-upload` (deliberate sabotage used to verify the CI image upload; never merge), `stage-0-tooling` (merged).
* **Local builds:** use the override with Clang 19 (`-DAXIOM_CLANG_VERSION=19 -DAXIOM_ALLOW_UNPINNED_COMPILER=ON`); Clang 18 cannot build `std::expected` with libstdc++ (ADR-0004).
1. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
2. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (Stage 0) | yes | n/a (hosted) | 0 (3 CI infra fixes) | n/a |
| 2026-09-28 | hosted Claude (Stage 1: 1a–7a) | yes | n/a (hosted) | 2 (committed with a failing check twice; commits are now gated on the full local run) | n/a |
