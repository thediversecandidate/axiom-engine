#include "axiom/renderer_vk/validation.hpp"
#include "axiom/renderer_vk/vulkan_device.hpp"
#include "axiom/renderer_vk/vulkan_instance.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string_view>

using namespace axiom::renderer;

namespace {
// The test environment selects lavapipe via VK_DRIVER_FILES (modules/renderer/CMakeLists.txt).
const DeviceDesc kLavapipe{VK_DRIVER_ID_MESA_LLVMPIPE};
} // namespace

TEST_CASE("lavapipe device is selected, identified and torn down with zero errors", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
    REQUIRE(instance.has_value());
    auto device = VulkanDevice::create(*instance, kLavapipe);
    REQUIRE(device.has_value());
    CHECK(device->handle() != VK_NULL_HANDLE);
    CHECK(device->graphicsQueue() != VK_NULL_HANDLE);
    CHECK(device->driverId() == VK_DRIVER_ID_MESA_LLVMPIPE);
  } // device destroyed, then instance
  CHECK(sink.errors.load() == 0);
}

TEST_CASE("the validation layer reports a real API misuse", "[renderer][stage1]") {
  ValidationSink sink;
  auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
  REQUIRE(instance.has_value());
  auto device = VulkanDevice::create(*instance, kLavapipe);
  REQUIRE(device.has_value());

  VkBufferCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size = 0; // invalid: VUID-VkBufferCreateInfo-size-00912 requires size > 0
  info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkBuffer buffer = VK_NULL_HANDLE;
  vkCreateBuffer(device->handle(), &info, nullptr, &buffer);
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device->handle(), buffer, nullptr);
  }
  CHECK(sink.errors.load() >= 1);
  CHECK(std::string_view(sink.firstError).find("00912") != std::string_view::npos);
}

TEST_CASE("a required driver that is not present is reported as unsupported", "[renderer][stage1]") {
  ValidationSink sink;
  auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
  REQUIRE(instance.has_value());
  auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_NVIDIA_PROPRIETARY, VK_NULL_HANDLE});
  REQUIRE_FALSE(device.has_value());
  CHECK(device.error().code == axiom::core::ErrorCode::kUnsupported);
}
