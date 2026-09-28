#include "axiom/renderer/offscreen_target.hpp"
#include "axiom/renderer/triangle_pipeline.hpp"
#include "axiom/renderer_vk/command_context.hpp"
#include "axiom/renderer_vk/host_buffer.hpp"
#include "axiom/renderer_vk/validation.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>

using namespace axiom::renderer;

namespace {

// Column-major MVP: the Stage 1 camera is the identity plus the projection's single Y inversion
// (CONVENTIONS §4). A world-space Y-up triangle therefore appears upright on screen.
constexpr float kYFlip[16] = {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

// Counter-clockwise when viewed from +Z (the glTF front-face convention).
constexpr std::array<TriangleVertex, 3> kTriangle{{
    {{-0.5f, -0.5f, 0.5f}, {1, 0, 0}},
    {{0.5f, -0.5f, 0.5f}, {0, 1, 0}},
    {{0.0f, 0.5f, 0.5f}, {0, 0, 1}},
}};

} // namespace

TEST_CASE("the triangle pipeline draws one triangle into the offscreen target", "[renderer][stage1]") {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink, {}});
    REQUIRE(instance.has_value());
    auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE, VK_NULL_HANDLE});
    REQUIRE(device.has_value());
    auto allocator = GpuAllocator::create(*instance, *device);
    REQUIRE(allocator.has_value());
    auto target = OffscreenTarget::create(*device, *allocator, 256, 256);
    REQUIRE(target.has_value());
    auto pipeline = TrianglePipeline::create(*device, OffscreenTarget::kFormat);
    REQUIRE(pipeline.has_value());
    auto vertices =
        HostBuffer::create(*allocator, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, std::as_bytes(std::span{kTriangle}));
    REQUIRE(vertices.has_value());
    auto commands = CommandContext::create(*device);
    REQUIRE(commands.has_value());

    auto cmd = commands->begin();
    REQUIRE(cmd.has_value());
    target->recordBegin(*cmd, {0.0f, 0.0f, 0.0f, 1.0f});
    pipeline->recordBind(*cmd, 256, 256, VK_FRONT_FACE_COUNTER_CLOCKWISE, kYFlip);
    const VkBuffer vb = vertices->handle();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(*cmd, 0, 1, &vb, &offset);
    vkCmdDraw(*cmd, 3, 1, 0, 0);
    target->recordEndAndReadback(*cmd);
    REQUIRE(commands->submitAndWait().has_value());

    const auto px = target->pixels();
    const Rgba8 center = px[128 * 256 + 128]; // inside the triangle
    const Rgba8 corner = px[5 * 256 + 5];     // outside: the clear color
    CHECK((center.r > 20 || center.g > 20 || center.b > 20));
    CHECK(center.a == 255);
    CHECK((corner.r == 0 && corner.g == 0 && corner.b == 0 && corner.a == 255));
    // Upright: the blue apex (world +Y) is near the top of the image, red/green near the bottom.
    const Rgba8 nearApex = px[80 * 256 + 128];
    const Rgba8 nearBase = px[180 * 256 + 128];
    CHECK(nearApex.b > nearApex.r);
    CHECK(nearBase.b < nearBase.r + nearBase.g);
  }
  CHECK(sink.errors.load() == 0);
  CHECK(sink.firstError[0] == '\0');
}
