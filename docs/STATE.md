# STATE

## Current
* **Stage:** 1 (Vertical Slice), branch `stage-1-triangle`. Stage 0 merged (see `docs/HISTORY.md`).
* **Done in Stage 1:** 1a `VulkanInstance` + validation sink; 1b `VulkanDevice` (lavapipe identity, explicit feature enables, a real validation error detected); 2a `GpuAllocator` (VMA); 2b `CommandContext` + `OffscreenTarget` (dynamic-rendering clear, readback, sRGB-encoded bytes match an independent oracle).
* **Next atomic step:** 3a — glslc shader build step in CMake (compile `.vert`/`.frag` to SPIR-V at build time, embed as `uint32_t` arrays) with a test that the SPIR-V is valid (renderer).
* **Remaining Stage 1 steps:** 3b triangle pipeline + glslc shader build; 4 landmark and coverage tests; 5 culling tests (incl. ordinary→mirrored→ordinary); 6 reference-image comparison + CI artifacts; 7 SDL3 swapchain path (local only).
* **Sizing rule (found in step 1):** new code per session must fit the working room (3,268 tokens ≈ 10 KB at the fallback rate), so steps are split to about one source file plus its test.

## Last modified
`modules/renderer/` (instance, device, allocator, command context, offscreen target + tests); toolchain + core build-info (libstdc++, ADR-0004); `scripts/gen_repo_map.py` (special-member compaction).

## Open items
* **Renderer map at 2,115 / 2,500 tokens:** split `renderer` into sub-libraries (e.g. `renderer_vk` for instance/device/allocator/commands) before it exceeds the budget.
* **libstdc++ + Clang 18 cannot build `std::expected`** (ADR-0004): verification now happens in CI only; the local override needs Clang 19+.
1. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
2. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (Stage 0) | yes | n/a (hosted) | 0 (3 CI infra fixes) | n/a |
| 2026-09-28 | hosted Claude (Stage 1: 1a–2b) | yes | n/a (hosted) | 1 (committed with a failing repo-map check; fixed) | n/a |
