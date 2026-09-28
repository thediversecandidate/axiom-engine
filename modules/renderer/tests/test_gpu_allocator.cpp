#include "axiom/renderer/gpu_allocator.hpp"
#include "axiom/renderer/validation.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace axiom::renderer;

TEST_CASE("VMA allocator allocates and frees a buffer with zero validation errors", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink});
    REQUIRE(instance.has_value());
    auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE});
    REQUIRE(device.has_value());
    auto allocator = GpuAllocator::create(*instance, *device);
    REQUIRE(allocator.has_value());

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = 4096;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    REQUIRE(vmaCreateBuffer(allocator->handle(), &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) == VK_SUCCESS);
    CHECK(buffer != VK_NULL_HANDLE);
    vmaDestroyBuffer(allocator->handle(), buffer, allocation);
  } // allocator, device, instance destroyed in reverse order
  CHECK(sink.errors.load() == 0);
}
