# STATE

## Current
* **Stage:** 1 (Vertical Slice), branch `stage-1-triangle`. Stage 0 merged (see `docs/HISTORY.md`).
* **Done in Stage 1:** 1a `VulkanInstance` + validation sink; 1b `VulkanDevice` (lavapipe identity, explicit dynamicRendering + synchronization2, a real validation error detected).
* **Next atomic step:** 2 — offscreen 256×256 `R8G8B8A8_SRGB` render target, command buffer, dynamic-rendering clear to (0,0,0,1), readback, zero validation errors (renderer).
* **Remaining Stage 1 steps:** 3 triangle pipeline + glslc shader build; 4 landmark and coverage tests; 5 culling tests (incl. ordinary→mirrored→ordinary); 6 reference-image comparison + CI artifacts; 7 SDL3 swapchain path (local only).
* **Sizing rule (found in step 1):** new code per session must fit the working room (3,268 tokens ≈ 10 KB at the fallback rate), so steps are split to about one source file plus its test.

## Last modified
`modules/renderer/`: `validation.hpp`, `vulkan_instance.{hpp,cpp}`, `vulkan_device.{hpp,cpp}`, tests, `CMakeLists.txt` (lavapipe test environment).

## Open items
1. **Tokenizer not pinned (ADR-0003):** `scripts/tokenizer/` is empty, so all counts use the conservative fallback (bytes/3). Needed: the local model's `tokenizer.json` and chat template, then a `tokenizer.lock`.
2. **Measured system-prompt tokens:** not yet measured for the local agent (default budget 3,500 assumed).

## Session metrics
| Date | Agent | Completed | Peak context | DoD failures | Peak VRAM |
| --- | --- | --- | --- | --- | --- |
| 2026-09-27 | hosted Claude (Stage 0) | yes | n/a (hosted) | 0 (3 CI infra fixes) | n/a |
| 2026-09-28 | hosted Claude (Stage 1: 1a, 1b) | yes | n/a (hosted) | 0 | n/a |
