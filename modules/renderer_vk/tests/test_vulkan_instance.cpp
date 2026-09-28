#include "axiom/renderer_vk/validation.hpp"
#include "axiom/renderer_vk/vulkan_instance.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

using axiom::renderer::InstanceDesc;
using axiom::renderer::ValidationSink;
using axiom::renderer::VulkanInstance;

TEST_CASE("Vulkan 1.3 instance with validation creates and tears down with zero errors", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
    REQUIRE(instance.has_value());
    CHECK(instance->handle() != VK_NULL_HANDLE);
    CHECK(instance->validationEnabled());
  } // destruction happens here; chained messenger also counts teardown messages
  CHECK(sink.errors.load() == 0);
  CHECK(sink.firstError[0] == '\0');
}

TEST_CASE("an ERROR message reaches the sink (the failure path is wired)", "[renderer][stage1]") {
  ValidationSink sink;
  auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
  REQUIRE(instance.has_value());
  instance->submitTestMessage(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "axiom deliberate error");
  instance->submitTestMessage(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "axiom deliberate warning");
  CHECK(sink.errors.load() == 1);
  CHECK(sink.warnings.load() >= 1);
  CHECK(std::strcmp(sink.firstError, "axiom deliberate error") == 0);
}

TEST_CASE("validation without a sink is rejected", "[renderer][stage1]") {
  auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, nullptr, {}});
  REQUIRE_FALSE(instance.has_value());
  CHECK(instance.error().code == axiom::core::ErrorCode::kInvalidArgument);
}

TEST_CASE("moving an instance transfers ownership exactly once", "[renderer][stage1]") {
  ValidationSink sink;
  auto created = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
  REQUIRE(created.has_value());
  VulkanInstance moved = std::move(*created);
  CHECK(created->handle() == VK_NULL_HANDLE);
  CHECK(moved.handle() != VK_NULL_HANDLE);
  moved = VulkanInstance{};
  CHECK(moved.handle() == VK_NULL_HANDLE);
  CHECK(sink.errors.load() == 0);
}
