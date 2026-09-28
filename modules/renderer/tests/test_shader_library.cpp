#include "axiom/renderer/shader_library.hpp"
#include "axiom/renderer/validation.hpp"
#include "axiom/renderer/vulkan_device.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace axiom::renderer;

namespace {
constexpr std::uint32_t kSpirvMagic = 0x07230203;
constexpr ShaderId kAllShaders[] = {ShaderId::kTriangleVert, ShaderId::kTriangleFrag};
} // namespace

TEST_CASE("built-in shaders are SPIR-V", "[renderer][stage1]") {
  for (ShaderId id : kAllShaders) {
    const auto words = shaderSpirv(id);
    REQUIRE(words.size() > 5); // header is 5 words
    CHECK(words[0] == kSpirvMagic);
    CHECK(words[1] >= 0x00010000); // SPIR-V version 1.x
  }
}

TEST_CASE("built-in shaders create Vulkan shader modules with zero validation errors", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink});
    REQUIRE(instance.has_value());
    auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE});
    REQUIRE(device.has_value());
    for (ShaderId id : kAllShaders) {
      const auto words = shaderSpirv(id);
      VkShaderModuleCreateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      info.codeSize = words.size_bytes();
      info.pCode = words.data();
      VkShaderModule module = VK_NULL_HANDLE;
      REQUIRE(vkCreateShaderModule(device->handle(), &info, nullptr, &module) == VK_SUCCESS);
      vkDestroyShaderModule(device->handle(), module, nullptr);
    }
  }
  CHECK(sink.errors.load() == 0); // the validation layer runs spirv-val on each module
  CHECK(sink.firstError[0] == '\0');
}

TEST_CASE("corrupted SPIR-V is rejected by the validation layer (the check is real)", "[renderer][stage1]") {
  ValidationSink sink;
  auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink});
  REQUIRE(instance.has_value());
  auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE});
  REQUIRE(device.has_value());

  const auto good = shaderSpirv(ShaderId::kTriangleFrag);
  std::vector<std::uint32_t> bad(good.begin(), good.end());
  bad[1] = 0x00020000u; // well-formed module, but SPIR-V version 2.0 does not exist
  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = bad.size() * sizeof(std::uint32_t);
  info.pCode = bad.data();
  VkShaderModule module = VK_NULL_HANDLE;
  vkCreateShaderModule(device->handle(), &info, nullptr, &module);
  if (module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device->handle(), module, nullptr);
  }
  CHECK(sink.errors.load() >= 1);
}
