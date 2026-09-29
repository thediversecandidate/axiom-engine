// Stage 1 step 7a: instance extensions and device presentation support, tested headlessly with
// VK_EXT_headless_surface on lavapipe (the SDL3 window path uses the same options with a real surface).

#include "axiom/renderer_vk/vulkan_device.hpp"
#include "axiom/renderer_vk/vulkan_instance.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

using namespace axiom;
using namespace axiom::renderer;

namespace {

constexpr std::array<const char *, 2> kHeadless{VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME};

VkSurfaceKHR createHeadlessSurface(const VulkanInstance &instance) {
  auto create = reinterpret_cast<PFN_vkCreateHeadlessSurfaceEXT>(
      vkGetInstanceProcAddr(instance.handle(), "vkCreateHeadlessSurfaceEXT"));
  REQUIRE(create != nullptr);
  VkHeadlessSurfaceCreateInfoEXT info{};
  info.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  REQUIRE(create(instance.handle(), &info, nullptr, &surface) == VK_SUCCESS);
  return surface;
}

} // namespace

TEST_CASE("a device created for a present surface enables VK_KHR_swapchain", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, kHeadless});
    REQUIRE(instance.has_value());
    const VkSurfaceKHR surface = createHeadlessSurface(*instance);
    {
      auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE, surface});
      REQUIRE(device.has_value());
      CHECK(device->presentEnabled());
      VkBool32 supported = VK_FALSE;
      REQUIRE(vkGetPhysicalDeviceSurfaceSupportKHR(device->physical(), device->graphicsQueueFamily(), surface,
                                                   &supported) == VK_SUCCESS);
      CHECK(supported == VK_TRUE);
      CHECK(vkGetDeviceProcAddr(device->handle(), "vkCreateSwapchainKHR") != nullptr);
    }
    vkDestroySurfaceKHR(instance->handle(), surface, nullptr);
  }
  CHECK(sink.errors.load() == 0);
}

TEST_CASE("a device created without a present surface does not enable presentation", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
    REQUIRE(instance.has_value());
    auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE, VK_NULL_HANDLE});
    REQUIRE(device.has_value());
    CHECK_FALSE(device->presentEnabled());
  }
  CHECK(sink.errors.load() == 0);
}

TEST_CASE("an unavailable instance extension is reported as unsupported", "[renderer][stage1]") {
  ValidationSink sink;
  const std::array<const char *, 1> bogus{"VK_AXIOM_no_such_extension"};
  auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, bogus});
  REQUIRE_FALSE(instance.has_value());
  CHECK(instance.error().code == core::ErrorCode::kUnsupported);
}
