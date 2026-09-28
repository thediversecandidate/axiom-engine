#include "axiom/renderer/gpu_allocator.hpp"

#include <utility>

namespace axiom::renderer {

core::Result<GpuAllocator> GpuAllocator::create(const VulkanInstance &instance, const VulkanDevice &device) {
  // VMA is built with dynamic function loading (third_party_impl/vma_impl.cpp).
  VmaVulkanFunctions functions{};
  functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo info{};
  info.vulkanApiVersion = VK_API_VERSION_1_3;
  info.instance = instance.handle();
  info.physicalDevice = device.physical();
  info.device = device.handle();
  info.pVulkanFunctions = &functions;

  GpuAllocator result;
  if (vmaCreateAllocator(&info, &result.allocator_) != VK_SUCCESS) {
    return core::fail(core::ErrorCode::kGpu, "vmaCreateAllocator failed");
  }
  return result;
}

GpuAllocator::GpuAllocator(GpuAllocator &&other) noexcept
    : allocator_(std::exchange(other.allocator_, VK_NULL_HANDLE)) {}

GpuAllocator &GpuAllocator::operator=(GpuAllocator &&other) noexcept {
  if (this != &other) {
    if (allocator_ != VK_NULL_HANDLE) {
      vmaDestroyAllocator(allocator_);
    }
    allocator_ = std::exchange(other.allocator_, VK_NULL_HANDLE);
  }
  return *this;
}

GpuAllocator::~GpuAllocator() {
  if (allocator_ != VK_NULL_HANDLE) {
    vmaDestroyAllocator(allocator_);
  }
}

} // namespace axiom::renderer
