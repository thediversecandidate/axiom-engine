// Proves the VMA and cgltf implementation libraries link, and that cgltf actually runs.
#include <catch2/catch_test_macros.hpp>
#include <cgltf.h>
#include <cstring>
#include <vk_mem_alloc.h>

TEST_CASE("VMA implementation symbols link", "[third_party][stage0]") {
  // vmaCreateAllocator needs a Vulkan device (Stage 1); here we prove the symbol resolves at link time.
  auto *createFn = &vmaCreateAllocator;
  auto *destroyFn = &vmaDestroyAllocator;
  CHECK(createFn != nullptr);
  CHECK(destroyFn != nullptr);
}

TEST_CASE("cgltf implementation parses a minimal glTF document", "[third_party][stage0]") {
  const char *json = R"({"asset":{"version":"2.0"}})";
  cgltf_options options{};
  cgltf_data *data = nullptr;
  const cgltf_result result = cgltf_parse(&options, json, std::strlen(json), &data);
  REQUIRE(result == cgltf_result_success);
  REQUIRE(data != nullptr);
  CHECK(std::strcmp(data->asset.version, "2.0") == 0);
  cgltf_free(data);
}
