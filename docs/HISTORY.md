# History (not loaded by default; read only when a task needs it)

## Stage 2 (in progress, branch `stage-2-math`)

### Steps verified in CI
* 2a `Vector3` (12 bytes) / `Vector4` (`alignas(16)`): arithmetic, dot, right-handed cross, overflow/underflow-safe `length`, `tryNormalize` (empty for non-finite or length < 1e-12), `normalized` asserted via math-private `AXIOM_MATH_ASSERT` (math cannot use core); fixture aborts in debug/asan-ubsan and returns zero in release; 10,000 seeded vectors normalize to unit length within 1e-6; a naive-length sabotage fails 3 checks

## Stage 1 (merged in PR #2, 2026-09-29)

### Steps verified in CI (all three presets + checks, Clang 21)
* 1a `VulkanInstance` + validation sink
* 1b `VulkanDevice` (lavapipe identity, explicit feature enables, a real validation error detected)
* 2a `GpuAllocator` (VMA)
* 2b `CommandContext` + `OffscreenTarget` (dynamic-rendering clear, readback, sRGB-encoded bytes match an independent oracle)
* 3a glslc build step + `shaderSpirv()` (valid modules, and a corrupted module is caught)
* 3b `TrianglePipeline` + `HostBuffer`, one CCW triangle drawn offscreen with zero validation errors
* 4 frozen scene `tests/golden/triangle.json` (perspective camera, apex farther than base), landmark colors vs. a CPU perspective-correct sRGB oracle (±2) and coverage vs. analytic area (±1%); an affine-interpolation sabotage is caught
* 5 culling: reversed winding draws 0 px; ordinary → mirrored (CW) → ordinary in one command buffer, each paired with its reversed copy, 1,050 px per triangle; a forced-CCW sabotage is caught
* 6 full-image comparison to the reviewed reference `tests/golden/triangle.reference.pam` (≤ 65 px beyond 2/255); debug/release/asan-ubsan byte-identical locally; affine sabotage gives 9,420 px beyond tolerance; the comparator's 65/66 boundary is unit-tested; actual/diff/reference PNGs uploaded by CI on failure
* 7a `InstanceDesc::extraExtensions` (missing extension → kUnsupported); `DeviceDesc::presentSurface` selects a graphics family that can present and enables VK_KHR_swapchain; tested with VK_EXT_headless_surface on lavapipe; a sabotage that skips enabling the extension is caught
* 7b `Swapchain` (renderer_present, ADR-0006): SRGB surface format, else UNORM images + mutable-format SRGB views (the headless surface offers only UNORM); FIFO; acquire/present report out-of-date as values; clear read back SRGB-encoded on every image, then recreate at a new extent, zero validation errors; a UNORM-view sabotage is caught (error 73 > 2)
* 7c `axiom_triangle` (SDL3 static, video only; optional X11 extensions off): window + surface via SDL, draws the triangle each frame, recreates on resize/out-of-date; `app.triangle_smoke` runs 3 frames with SDL's offscreen driver on lavapipe with validation; a missing acquire-semaphore wait gives 5 validation errors and exit 1
* Reference image approved visually by Derrick (2026-09-28)
* Renderer split into `renderer_vk` + `renderer` when the renderer map reached 2,826 / 2,500 tokens (ADR-0005)

## Stage 0 (merged in PR #1, 2026-09-28)

### Verified locally (Clang 18 via the local-only override; not a Definition-of-Done result)
* `debug`, `asan-ubsan`, `release` build; all tests pass (7 / 11 / 7).
* UBSan and LeakSanitizer fixtures fail with their expected reports; clean controls pass.
* 33 script self-tests pass, including the timeout test with a SIGTERM-ignoring grandchild.
* `check_all.py` clean; lavapipe check passes with validation layers installed.

### Verified in CI (Definition of Done, PR #1)
* CI image: Ubuntu 26.04 (digest-pinned), apt snapshot 20260927T000000Z; Clang 21.1.8, CMake 4.2.3, Python 3.14.4.
* `debug`, `asan-ubsan`, `release`: first configure with an empty dependency cache, build and all tests pass on Clang 21 + libc++ 21.
* `check_all.py` (includes, line limit, repo maps, 33 self-tests, clang-format 21) passes; lavapipe selected, llvmpipe reported, validation layer present.

### Found during Stage 0
* First CI run: the HTTPS-only snapshot service needs `ca-certificates` bootstrapped into the bare base image (ADR-0002).
* CI logs are unreachable from the authoring environment, so every CI step reports failures and key facts as annotations (`scripts/ci_annotate.py`).
* The first real pre-flight refused an ordinary session: the always-loaded docs were 3,735 of 3,000 tokens under the fallback counter. Fixed by splitting `CONTEXT_RULES.md` (ADR-0003); now 2,409 of 3,000.

## Stage 1 findings
* **libc++ vs libstdc++ (ADR-0004):** under ASan, the first GPU submit showed libstdc++ exceptions inside the validation layer being destroyed by libc++abi. Switched to libstdc++.
* **Validation-layer shader cache:** the layer's on-disk cache (`~/.cache/shader_validation_cache-0.bin`) skipped re-validating shader modules seen in earlier runs, so a deliberate-invalid-SPIR-V test passed on a fresh machine (CI) and failed on the second local run. `VulkanInstance` now disables `check_shaders_caching` via `VK_EXT_layer_settings`, and `renderer.unit.second_run` runs the suite twice in CI.
