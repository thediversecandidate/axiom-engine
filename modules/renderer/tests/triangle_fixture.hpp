#pragma once
// Test-only: renders triangle draws into a fresh 256x256 OffscreenTarget on lavapipe in ONE command buffer
// and returns the pixels and the validation error count (including teardown).

#include "axiom/renderer/offscreen_target.hpp"
#include "axiom/renderer/triangle_pipeline.hpp"
#include "axiom/renderer_vk/command_context.hpp"
#include "axiom/renderer_vk/host_buffer.hpp"
#include "axiom/renderer_vk/validation.hpp"
#include "golden_reader.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace axiom::test {

struct TriangleDraw {
  float mvp[16]; // column-major
  VkFrontFace frontFace;
  std::uint32_t firstVertex;
};

struct RenderedFrame {
  std::vector<renderer::Rgba8> pixels;
  std::uint32_t validationErrors;
};

inline constexpr std::uint32_t kFrameSize = 256;

// The frozen Stage 1 scene from tests/golden/triangle.json: camera MVP and the three vertices.
struct TriangleScene {
  float mvp[16]; // column-major
  std::array<renderer::TriangleVertex, 3> vertices;
};

inline TriangleScene loadTriangleScene(const std::string &json) {
  TriangleScene scene{};
  const auto mvp = goldenNumbers(json, "mvp_column_major");
  REQUIRE(mvp.size() == 16);
  for (int i = 0; i < 16; ++i) {
    scene.mvp[i] = static_cast<float>(mvp[i]);
  }
  const auto v = goldenNumbers(json, "vertices");
  REQUIRE(v.size() == 18);
  for (int i = 0; i < 3; ++i) {
    for (int k = 0; k < 3; ++k) {
      scene.vertices[i].position[k] = static_cast<float>(v[i * 6 + k]);
      scene.vertices[i].color[k] = static_cast<float>(v[i * 6 + 3 + k]);
    }
  }
  return scene;
}

inline RenderedFrame renderTriangles(std::span<const renderer::TriangleVertex> vertices,
                                     std::span<const TriangleDraw> draws) {
  using namespace axiom::renderer;
  ValidationSink sink;
  RenderedFrame frame{};
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
    REQUIRE(instance.has_value());
    auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE, VK_NULL_HANDLE});
    REQUIRE(device.has_value());
    auto allocator = GpuAllocator::create(*instance, *device);
    REQUIRE(allocator.has_value());
    auto target = OffscreenTarget::create(*device, *allocator, kFrameSize, kFrameSize);
    REQUIRE(target.has_value());
    auto pipeline = TrianglePipeline::create(*device, OffscreenTarget::kFormat);
    REQUIRE(pipeline.has_value());
    auto vbo = HostBuffer::create(*allocator, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, std::as_bytes(vertices));
    REQUIRE(vbo.has_value());
    auto commands = CommandContext::create(*device);
    REQUIRE(commands.has_value());

    auto cmd = commands->begin();
    REQUIRE(cmd.has_value());
    target->recordBegin(*cmd, {0.0f, 0.0f, 0.0f, 1.0f});
    const VkBuffer vb = vbo->handle();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(*cmd, 0, 1, &vb, &offset);
    for (const TriangleDraw &d : draws) {
      pipeline->recordBind(*cmd, kFrameSize, kFrameSize, d.frontFace, d.mvp);
      vkCmdDraw(*cmd, 3, 1, d.firstVertex, 0);
    }
    target->recordEndAndReadback(*cmd);
    REQUIRE(commands->submitAndWait().has_value());
    const auto px = target->pixels();
    frame.pixels.assign(px.begin(), px.end());
  }
  frame.validationErrors = sink.errors.load();
  return frame;
}

} // namespace axiom::test
