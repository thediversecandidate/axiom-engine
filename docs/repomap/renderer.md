# Module `renderer` — public API
Depends on: core, math, renderer_vk

## `modules/renderer/include/axiom/renderer/module_info.hpp` — Stage 0 placeholder API for the renderer module. Allowed dependencies: axiom::core axiom::math.
- `[[nodiscard]] std::string_view moduleName() noexcept`
  /// Name of this module, used by the Stage 0 smoke test.

## `modules/renderer/include/axiom/renderer/offscreen_target.hpp` — Offscreen color target for headless rendering and CI image tests (ROADMAP Stage 1):
- `struct Rgba8`
  - `std::uint8_t r, g, b, a`
- `class OffscreenTarget`
  /// @owns the color image, its view and a readback buffer (all VMA allocations); move-only.
  /// @lifetime must be destroyed before the GpuAllocator it was created with.
  /// @thread single-threaded.
  /// @errors create() fails with kGpu if an allocation or view creation fails.
  - `(move-only, default-constructible)`
  - `static constexpr VkFormat kFormat = VK_FORMAT_R8G8B8A8_SRGB`
  - `[[nodiscard]] static core::Result<OffscreenTarget> create(const VulkanDevice &device, const GpuAllocator &allocator, std::uint32_t width, std::uint32_t height)`
  - `void recordBegin(VkCommandBuffer cmd, const float (&clearLinear)[4]) const noexcept`
  - `void recordEndAndReadback(VkCommandBuffer cmd) const noexcept`
  - `[[nodiscard]] std::span<const Rgba8> pixels() const noexcept`
  - `[[nodiscard]] std::uint32_t width() const noexcept`
  - `[[nodiscard]] std::uint32_t height() const noexcept`
  - `void reset() noexcept`
  - `VkDevice device_ = VK_NULL_HANDLE`
  - `VmaAllocator allocator_ = VK_NULL_HANDLE`
  - `VkImage image_ = VK_NULL_HANDLE`
  - `VmaAllocation imageAlloc_ = nullptr`
  - `VkImageView view_ = VK_NULL_HANDLE`
  - `VkBuffer readback_ = VK_NULL_HANDLE`
  - `VmaAllocation readbackAlloc_ = nullptr`
  - `void *mapped_ = nullptr`
  - `std::uint32_t width_ = 0`
  - `std::uint32_t height_ = 0`

## `modules/renderer/include/axiom/renderer/shader_library.hpp` — SPIR-V compiled from modules/renderer/shaders at build time (cmake/AxiomShaders.cmake).
- `enum class ShaderId : std::uint8_t`
  - `kTriangleVert, kTriangleFrag`
- `[[nodiscard]] std::span<const std::uint32_t> shaderSpirv(ShaderId id) noexcept`
  /// SPIR-V words of a built-in shader.
  /// @lifetime static storage; the span is valid for the whole program.

## `modules/renderer/include/axiom/renderer/triangle_pipeline.hpp` — Stage 1 triangle pipeline: dynamic rendering, vertex = {vec3 position, vec3 linear color},
- `struct TriangleVertex`
  - `float position[3]`
  - `float color[3]`
- `class TrianglePipeline`
  /// @owns the VkPipeline and its VkPipelineLayout; move-only.
  /// @lifetime must be destroyed before the device.
  /// @errors create() fails with kGpu if shader-module, layout or pipeline creation fails.
  - `(move-only, default-constructible)`
  - `[[nodiscard]] static core::Result<TrianglePipeline> create(const VulkanDevice &device, VkFormat colorFormat)`
  - `void recordBind(VkCommandBuffer cmd, std::uint32_t width, std::uint32_t height, VkFrontFace frontFace, const float (&mvpColumnMajor)[16]) const noexcept`
  - `void reset() noexcept`
  - `VkDevice device_ = VK_NULL_HANDLE`
  - `VkPipelineLayout layout_ = VK_NULL_HANDLE`
  - `VkPipeline pipeline_ = VK_NULL_HANDLE`
