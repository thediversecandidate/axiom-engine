// Stage 1 step 7b: Swapchain on lavapipe via VK_EXT_headless_surface. The headless surface offers only
// UNORM formats, so this also covers the mutable-format path (UNORM images rendered through SRGB views).

#include "axiom/renderer_present/swapchain.hpp"
#include "axiom/renderer_vk/command_context.hpp"
#include "axiom/renderer_vk/gpu_allocator.hpp"
#include "axiom/renderer_vk/validation.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>

using namespace axiom;
using namespace axiom::renderer;

namespace {

constexpr std::array<const char *, 2> kHeadless{VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME};

VkSurfaceKHR createHeadlessSurface(VkInstance instance) {
  auto create =
      reinterpret_cast<PFN_vkCreateHeadlessSurfaceEXT>(vkGetInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT"));
  REQUIRE(create != nullptr);
  VkHeadlessSurfaceCreateInfoEXT info{};
  info.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  REQUIRE(create(instance, &info, nullptr, &surface) == VK_SUCCESS);
  return surface;
}

// Host-readable (random access) readback buffer, owned by the test.
struct Readback {
  VmaAllocator allocator = VK_NULL_HANDLE;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = nullptr;
  const std::uint8_t *bytes = nullptr;
  Readback(const GpuAllocator &gpu, VkDeviceSize size) : allocator(gpu.handle()) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_AUTO;
    alloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo mapped{};
    REQUIRE(vmaCreateBuffer(allocator, &info, &alloc, &buffer, &allocation, &mapped) == VK_SUCCESS);
    bytes = static_cast<const std::uint8_t *>(mapped.pMappedData);
  }
  ~Readback() { vmaDestroyBuffer(allocator, buffer, allocation); }
  Readback(const Readback &) = delete;
  Readback &operator=(const Readback &) = delete;
};

} // namespace

TEST_CASE("the swapchain clears SRGB-encoded frames, presents them and survives recreation", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, kHeadless});
    REQUIRE(instance.has_value());
    const VkSurfaceKHR surface = createHeadlessSurface(instance->handle());
    {
      auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE, surface});
      REQUIRE(device.has_value());
      auto allocator = GpuAllocator::create(*instance, *device);
      REQUIRE(allocator.has_value());
      auto commands = CommandContext::create(*device);
      REQUIRE(commands.has_value());
      auto swapchain = Swapchain::create(*device, SwapchainDesc{surface, 64, 48, VK_IMAGE_USAGE_TRANSFER_SRC_BIT});
      REQUIRE(swapchain.has_value());
      CHECK(swapchain->extent().width == 64);
      CHECK(swapchain->extent().height == 48);
      const VkFormat view = swapchain->viewFormat();
      CHECK((view == VK_FORMAT_B8G8R8A8_SRGB || view == VK_FORMAT_R8G8B8A8_SRGB));
      CHECK(swapchain->imageCount() >= 2);

      Readback readback(*allocator, 64 * 48 * 4);
      // Same oracle as the offscreen clear test: linear (0.5, 0.25, 1.0, 0.5) → sRGB bytes (188, 137, 255), alpha 128.
      const bool bgra = view == VK_FORMAT_B8G8R8A8_SRGB;
      const std::array<int, 4> expected = bgra ? std::array{255, 137, 188, 128} : std::array{188, 137, 255, 128};
      for (std::uint32_t i = 0; i < swapchain->imageCount() + 1; ++i) { // wraps around every image once
        auto frame = swapchain->acquire();
        REQUIRE(frame.has_value());
        REQUIRE(frame->has_value());
        auto cmd = commands->begin();
        REQUIRE(cmd.has_value());
        swapchain->recordBegin(*cmd, **frame, {0.5f, 0.25f, 1.0f, 0.5f});
        swapchain->recordEnd(*cmd, **frame, readback.buffer);
        REQUIRE(commands->submitAndWait((*frame)->acquired, (*frame)->renderFinished).has_value());
        int worst = 0;
        for (std::size_t p = 0; p < 64 * 48; ++p) {
          for (int c = 0; c < 4; ++c) {
            worst = std::max(worst, std::abs(readback.bytes[p * 4 + c] - expected[c]));
          }
        }
        CHECK(worst <= 2);
        auto presented = swapchain->present(**frame);
        REQUIRE(presented.has_value());
        CHECK(*presented);
      }

      REQUIRE(swapchain->recreate(128, 64).has_value());
      CHECK(swapchain->extent().width == 128);
      auto frame = swapchain->acquire();
      REQUIRE(frame.has_value());
      REQUIRE(frame->has_value());
      auto cmd = commands->begin();
      REQUIRE(cmd.has_value());
      swapchain->recordBegin(*cmd, **frame, {0.0f, 0.0f, 0.0f, 1.0f});
      swapchain->recordEnd(*cmd, **frame);
      REQUIRE(commands->submitAndWait((*frame)->acquired, (*frame)->renderFinished).has_value());
      REQUIRE(swapchain->present(**frame).has_value());
    }
    vkDestroySurfaceKHR(instance->handle(), surface, nullptr);
  }
  CHECK(sink.errors.load() == 0);
  CHECK(sink.firstError[0] == '\0');
}

TEST_CASE("a swapchain needs a device created for presentation", "[renderer][stage1]") {
  ValidationSink sink;
  auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
  REQUIRE(instance.has_value());
  auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE, VK_NULL_HANDLE});
  REQUIRE(device.has_value());
  auto swapchain = Swapchain::create(*device, SwapchainDesc{VK_NULL_HANDLE, 64, 48, 0});
  REQUIRE_FALSE(swapchain.has_value());
  CHECK(swapchain.error().code == core::ErrorCode::kInvalidArgument);
}
