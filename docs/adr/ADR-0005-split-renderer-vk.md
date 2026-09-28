# ADR-0005: Split the renderer module (renderer_vk + renderer)

**Status:** accepted (2026-09-28)

## Context
After Stage 1 step 3b, the `renderer` repo map measured 2,826 tokens against its 2,500 budget (AX-MAP-002). CONTEXT_TOOLING §2 requires splitting an over-budget module by responsibility rather than trimming its map.

## Decision
* `renderer_vk` (depends on core, math) holds the Vulkan foundation: `ValidationSink`, `VulkanInstance`, `VulkanDevice`, `GpuAllocator`, `CommandContext`, `HostBuffer`.
* `renderer` (depends on core, math, renderer_vk) holds drawing: `OffscreenTarget`, the shader library, `TrianglePipeline`.
* `renderer_vk` may never include `renderer`. `app` may use both. Both keep the `axiom::renderer` namespace.

## Consequences
Sessions working on drawing load the `renderer_vk` public headers as dependency headers (a separate budget slot). Enforced by `check_includes.py` and the CMake link graph.
