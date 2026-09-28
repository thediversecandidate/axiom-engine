# STATE

## Current
* **Stage:** 1 (Vertical Slice), branch `stage-1-triangle`. Stage 0 merged (see `docs/HISTORY.md`).
* **Done in Stage 1:** 1a `VulkanInstance` + validation sink; 1b `VulkanDevice` (lavapipe identity, explicit feature enables, a real validation error detected); 2a `GpuAllocator` (VMA); 2b `CommandContext` + `OffscreenTarget` (dynamic-rendering clear, readback, sRGB-encoded bytes match an independent oracle); 3a glslc build step + `shaderSpirv()` (valid modules, and a corrupted module is caught).
* **Next atomic step:** 3b — graphics pipeline (dynamic rendering, vertex layout pos+color, MVP push constant, dynamic front face, CCW/back-face culling) and one triangle drawn into `OffscreenTarget` (renderer).
* **Remaining Stage 1 steps:** 4 landmark and coverage tests; 5 culling tests (incl. ordinary→mirrored→ordinary); 6 reference-image comparison + CI artifacts; 7 SDL3 swapchain path (local only).
* **Sizing rule (found in step 1):** new code per session must fit the working room (3,268 tokens ≈ 10 KB at the fallback rate), so steps are split to about one source file plus its test.

## Last modified
`modules/renderer/` (instance, device, allocator, command context, offscreen target + tests); toolchain + core build-info (libstdc++, ADR-0004); `scripts/gen_repo_map.py` (special-member compaction).

## Open items
* **Renderer map at 2,115 / 2,500 tokens:** split `renderer` into sub-libraries (e.g. `renderer_vk` for instance/device/allocator/commands) before it exceeds the budget.
* **Local builds:** use the override with Clang 19 (`-DAXIOM_CLANG_VERSION=19 -DAXIOM_ALLOW_UNPINNED_COMPILER=ON`); Clang 18 cannot build `std::expected` with libstdc++ (ADR-0004).
1. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
2. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (Stage 0) | yes | n/a (hosted) | 0 (3 CI infra fixes) | n/a |
| 2026-09-28 | hosted Claude (Stage 1: 1a–3a) | yes | n/a (hosted) | 2 (committed with a failing check twice; commits are now gated on the full local run) | n/a |
