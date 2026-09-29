#pragma once
// Offscreen color target for headless rendering and CI image tests (ROADMAP Stage 1):
// R8G8B8A8_SRGB image + host-readable readback buffer, rendered with dynamic rendering.

#include "axiom/core/error.hpp"
#include "axiom/renderer_vk/gpu_allocator.hpp"

#include <cstdint>
#include <span>
#include <vulkan/vulkan.h>

namespace axiom::renderer {

struct Rgba8 {
  std::uint8_t r, g, b, a;
};

/// @owns the color image, its view and a readback buffer (all VMA allocations); move-only.
/// @lifetime must be destroyed before the GpuAllocator it was created with.
/// @thread single-threaded.
/// @errors create() fails with kGpu if an allocation or view creation fails.
class OffscreenTarget {
public:
  static constexpr VkFormat kFormat = VK_FORMAT_R8G8B8A8_SRGB;

  [[nodiscard]] static core::Result<OffscreenTarget> create(const VulkanDevice &device, const GpuAllocator &allocator,
                                                            std::uint32_t width, std::uint32_t height);

  OffscreenTarget() = default;
  OffscreenTarget(OffscreenTarget &&other) noexcept;
  OffscreenTarget &operator=(OffscreenTarget &&other) noexcept;
  OffscreenTarget(const OffscreenTarget &) = delete;
  OffscreenTarget &operator=(const OffscreenTarget &) = delete;
  ~OffscreenTarget();

  /// Records: layout → color attachment, vkCmdBeginRendering clearing to `clearLinear` (linear RGBA).
  void recordBegin(VkCommandBuffer cmd, const float (&clearLinear)[4]) const noexcept;
  /// Records: vkCmdEndRendering, layout → transfer source, copy into the readback buffer.
  void recordEndAndReadback(VkCommandBuffer cmd) const noexcept;
  /// Pixels in row-major order; valid after the recorded commands have completed on the GPU.
  [[nodiscard]] std::span<const Rgba8> pixels() const noexcept;

  [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
  [[nodiscard]] std::uint32_t height() const noexcept { return height_; }

private:
  void reset() noexcept;

  VkDevice device_ = VK_NULL_HANDLE;
  VmaAllocator allocator_ = VK_NULL_HANDLE;
  VkImage image_ = VK_NULL_HANDLE;
  VmaAllocation imageAlloc_ = nullptr;
  VkImageView view_ = VK_NULL_HANDLE;
  VkBuffer readback_ = VK_NULL_HANDLE;
  VmaAllocation readbackAlloc_ = nullptr;
  void *mapped_ = nullptr;
  std::uint32_t width_ = 0;
  std::uint32_t height_ = 0;
};

} // namespace axiom::renderer
