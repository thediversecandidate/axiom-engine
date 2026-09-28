#include "axiom/renderer/offscreen_target.hpp"
#include "axiom/renderer_vk/command_context.hpp"
#include "axiom/renderer_vk/validation.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdlib>

using namespace axiom::renderer;

namespace {

// Independent oracle: the sRGB transfer function (IEC 61966-2-1), quantized to 8 bits.
int srgbByte(float linear) {
  const float c = linear <= 0.0031308f ? 12.92f * linear : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
  return static_cast<int>(std::lround(c * 255.0f));
}

// Clears a 256x256 target to `clear` (linear RGBA) and checks every pixel against `expected` within 2/255.
void clearAndCheck(const float (&clear)[4], Rgba8 expected) {
  ValidationSink sink;
  {
    auto instance = VulkanInstance::create(InstanceDesc{"axiom-test", true, &sink});
    REQUIRE(instance.has_value());
    auto device = VulkanDevice::create(*instance, DeviceDesc{VK_DRIVER_ID_MESA_LLVMPIPE});
    REQUIRE(device.has_value());
    auto allocator = GpuAllocator::create(*instance, *device);
    REQUIRE(allocator.has_value());
    auto target = OffscreenTarget::create(*device, *allocator, 256, 256);
    REQUIRE(target.has_value());
    auto commands = CommandContext::create(*device);
    REQUIRE(commands.has_value());

    auto cmd = commands->begin();
    REQUIRE(cmd.has_value());
    target->recordBegin(*cmd, clear);
    target->recordEndAndReadback(*cmd);
    REQUIRE(commands->submitAndWait().has_value());

    const auto px = target->pixels();
    REQUIRE(px.size() == 256u * 256u);
    int worst = 0;
    for (const Rgba8 &p : px) {
      worst = std::max({worst, std::abs(p.r - expected.r), std::abs(p.g - expected.g), std::abs(p.b - expected.b),
                        std::abs(p.a - expected.a)});
    }
    CHECK(worst <= 2);
  }
  CHECK(sink.errors.load() == 0);
}

} // namespace

TEST_CASE("offscreen clear to opaque black reads back as (0,0,0,255)", "[renderer][stage1]") {
  clearAndCheck({0.0f, 0.0f, 0.0f, 1.0f}, Rgba8{0, 0, 0, 255});
}

TEST_CASE("offscreen clear stores sRGB-encoded color and linear alpha", "[renderer][stage1]") {
  const float clear[4] = {0.5f, 0.25f, 1.0f, 0.5f};
  const Rgba8 expected{static_cast<std::uint8_t>(srgbByte(0.5f)), static_cast<std::uint8_t>(srgbByte(0.25f)),
                       static_cast<std::uint8_t>(srgbByte(1.0f)), static_cast<std::uint8_t>(std::lround(0.5f * 255))};
  CHECK(expected.r == 188); // sanity check of the oracle itself: 0.5 linear -> 188 sRGB
  clearAndCheck(clear, expected);
}
