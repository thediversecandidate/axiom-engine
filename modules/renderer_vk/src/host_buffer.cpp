#include "axiom/renderer_vk/host_buffer.hpp"

#include <cstring>
#include <utility>

namespace axiom::renderer {

core::Result<HostBuffer> HostBuffer::create(const GpuAllocator &allocator, VkBufferUsageFlags usage,
                                            std::span<const std::byte> data) {
  if (data.empty()) {
    return core::fail(core::ErrorCode::kInvalidArgument, "HostBuffer data is empty");
  }
  VkBufferCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size = data.size();
  info.usage = usage;
  VmaAllocationCreateInfo alloc{};
  alloc.usage = VMA_MEMORY_USAGE_AUTO;
  alloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  HostBuffer result;
  result.allocator_ = allocator.handle();
  VmaAllocationInfo mapped{};
  if (vmaCreateBuffer(result.allocator_, &info, &alloc, &result.buffer_, &result.allocation_, &mapped) != VK_SUCCESS) {
    return core::fail(core::ErrorCode::kGpu, "HostBuffer vmaCreateBuffer failed");
  }
  std::memcpy(mapped.pMappedData, data.data(), data.size());
  vmaFlushAllocation(result.allocator_, result.allocation_, 0, VK_WHOLE_SIZE);
  return result;
}

HostBuffer::HostBuffer(HostBuffer &&other) noexcept
    : allocator_(std::exchange(other.allocator_, VK_NULL_HANDLE)),
      buffer_(std::exchange(other.buffer_, VK_NULL_HANDLE)), allocation_(std::exchange(other.allocation_, nullptr)) {}

HostBuffer &HostBuffer::operator=(HostBuffer &&other) noexcept {
  if (this != &other) {
    if (buffer_ != VK_NULL_HANDLE) {
      vmaDestroyBuffer(allocator_, buffer_, allocation_);
    }
    allocator_ = std::exchange(other.allocator_, VK_NULL_HANDLE);
    buffer_ = std::exchange(other.buffer_, VK_NULL_HANDLE);
    allocation_ = std::exchange(other.allocation_, nullptr);
  }
  return *this;
}

HostBuffer::~HostBuffer() {
  if (buffer_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
  }
}

} // namespace axiom::renderer
