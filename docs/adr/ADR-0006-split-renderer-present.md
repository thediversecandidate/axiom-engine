# ADR-0006: Presentation module (renderer_present)

**Status:** accepted (2026-09-28)

## Context
Adding `Swapchain` (Stage 1 step 7b) to `renderer_vk` raised its repo map to 2,856 tokens against the 2,500 budget (AX-MAP-002). CONTEXT_TOOLING §2 requires splitting by responsibility rather than trimming the map.

## Decision
* `renderer_present` (depends on core, math, renderer_vk) holds presentation to a surface: `Swapchain`.
* `renderer_vk` keeps what presentation needs from the device: `InstanceDesc::extraExtensions`, `DeviceDesc::presentSurface`, `VulkanDevice::presentEnabled()` / `mutableSwapchainFormat()`, and the semaphore parameters of `CommandContext::submitAndWait`.
* `renderer` does not depend on `renderer_present`: pipelines take a color format, so drawing code is the same for offscreen targets and swapchain images. `app` may use all three. The namespace stays `axiom::renderer`.
* When a surface offers no SRGB format (e.g. `VK_EXT_headless_surface`), the swapchain uses UNORM images with `VK_KHR_swapchain_mutable_format` and SRGB views, so output stays SRGB-encoded (CONVENTIONS §4). Without that extension, creation fails with kUnsupported.

## Consequences
The swapchain is tested in CI on lavapipe through the headless surface; the SDL3 window path (local only) uses the same classes with a real surface. Enforced by `check_includes.py` and the CMake link graph.
