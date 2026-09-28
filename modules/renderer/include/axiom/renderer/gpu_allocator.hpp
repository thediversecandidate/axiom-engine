#pragma once
// GPU memory allocation through the Vulkan Memory Allocator (CONVENTIONS §1).

#include "axiom/core/error.hpp"
#include "axiom/renderer/vulkan_device.hpp"
#include "axiom/renderer/vulkan_instance.hpp"

#include <vk_mem_alloc.h>

namespace axiom::renderer {

/// @owns the VmaAllocator; move-only.
/// @lifetime destroy every allocation made with it first; must be destroyed before the device.
/// @errors create() fails with kGpu if vmaCreateAllocator fails.
class GpuAllocator {
public:
  [[nodiscard]] static core::Result<GpuAllocator> create(const VulkanInstance &instance, const VulkanDevice &device);

  GpuAllocator() = default;
  GpuAllocator(GpuAllocator &&other) noexcept;
  GpuAllocator &operator=(GpuAllocator &&other) noexcept;
  GpuAllocator(const GpuAllocator &) = delete;
  GpuAllocator &operator=(const GpuAllocator &) = delete;
  ~GpuAllocator();

  [[nodiscard]] VmaAllocator handle() const noexcept { return allocator_; }

private:
  VmaAllocator allocator_ = VK_NULL_HANDLE;
};

} // namespace axiom::renderer
