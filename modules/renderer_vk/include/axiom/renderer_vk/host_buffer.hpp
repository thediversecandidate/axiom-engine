#pragma once
// Host-visible GPU buffer filled from CPU memory (vertex data, small uploads).

#include "axiom/core/error.hpp"
#include "axiom/renderer_vk/gpu_allocator.hpp"

#include <cstddef>
#include <span>
#include <vulkan/vulkan.h>

namespace axiom::renderer {

/// @owns a VMA buffer allocation; move-only.
/// @lifetime must be destroyed before the GpuAllocator it was created with.
/// @errors create() fails with kGpu if the allocation fails, kInvalidArgument for empty data.
class HostBuffer {
public:
  [[nodiscard]] static core::Result<HostBuffer> create(const GpuAllocator &allocator, VkBufferUsageFlags usage,
                                                       std::span<const std::byte> data);

  HostBuffer() = default;
  HostBuffer(HostBuffer &&other) noexcept;
  HostBuffer &operator=(HostBuffer &&other) noexcept;
  HostBuffer(const HostBuffer &) = delete;
  HostBuffer &operator=(const HostBuffer &) = delete;
  ~HostBuffer();

  [[nodiscard]] VkBuffer handle() const noexcept { return buffer_; }

private:
  VmaAllocator allocator_ = VK_NULL_HANDLE;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = nullptr;
};

} // namespace axiom::renderer
