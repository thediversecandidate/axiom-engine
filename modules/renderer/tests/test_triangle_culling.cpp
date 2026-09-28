// ROADMAP Stage 1 acceptance: back-face culling and per-draw front face (CONVENTIONS §4).
// A mirrored world transform (negative determinant) is drawn with CLOCKWISE; every other draw uses
// COUNTER_CLOCKWISE. vkCmdSetFrontFace is recorded per draw inside one command buffer.

#include "triangle_fixture.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

using namespace axiom;

namespace {

constexpr float kScale = 0.3f;         // three triangles fit side by side
constexpr std::uint32_t kReversed = 3; // first vertex of the reversed-winding copy (0, 2, 1)

struct Setup {
  test::TriangleScene scene;
  std::array<renderer::TriangleVertex, 6> vertices;
};

Setup load() {
  Setup s{test::loadTriangleScene(test::readGolden("triangle.json")), {}};
  const auto &v = s.scene.vertices;
  s.vertices = {v[0], v[1], v[2], v[0], v[2], v[1]};
  return s;
}

// MVP * T(tx, 0, 0) * diag(mirrorX * kScale, kScale, kScale), column-major.
test::TriangleDraw draw(const Setup &s, float tx, float mirrorX, VkFrontFace face, std::uint32_t first) {
  test::TriangleDraw d{};
  const float *pv = s.scene.mvp;
  for (int r = 0; r < 4; ++r) {
    d.mvp[0 + r] = pv[0 + r] * mirrorX * kScale;
    d.mvp[4 + r] = pv[4 + r] * kScale;
    d.mvp[8 + r] = pv[8 + r] * kScale;
    d.mvp[12 + r] = pv[0 + r] * tx + pv[12 + r];
  }
  d.frontFace = face;
  d.firstVertex = first;
  return d;
}

// Foreground pixels in the left, middle and right thirds of the frame.
std::array<int, 3> countByThird(const test::RenderedFrame &f) {
  std::array<int, 3> n{};
  for (std::uint32_t y = 0; y < test::kFrameSize; ++y) {
    for (std::uint32_t x = 0; x < test::kFrameSize; ++x) {
      const auto &p = f.pixels[y * test::kFrameSize + x];
      if ((p.r | p.g | p.b) != 0) {
        ++n[x * 3 / test::kFrameSize];
      }
    }
  }
  return n;
}

constexpr auto kCcw = VK_FRONT_FACE_COUNTER_CLOCKWISE;
constexpr auto kCw = VK_FRONT_FACE_CLOCKWISE;

} // namespace

TEST_CASE("the front-facing triangle is drawn and its reversed-winding copy is culled", "[renderer][stage1]") {
  const Setup s = load();
  const std::array front{draw(s, 0.0f, 1.0f, kCcw, 0)};
  const std::array back{draw(s, 0.0f, 1.0f, kCcw, kReversed)};
  const auto drawn = test::renderTriangles(s.vertices, front);
  const auto culled = test::renderTriangles(s.vertices, back);
  CHECK(countByThird(drawn)[1] > 100);
  CHECK(countByThird(culled) == std::array<int, 3>{0, 0, 0});
  CHECK(drawn.validationErrors == 0);
  CHECK(culled.validationErrors == 0);
}

TEST_CASE("ordinary, mirrored, ordinary in one command buffer all cull correctly", "[renderer][stage1]") {
  const Setup s = load();
  // Each position also draws its reversed copy, which must be culled, so a wrong front face shows up
  // either as a missing triangle or as a doubled one.
  const std::array draws{
      draw(s, -0.7f, 1.0f, kCcw, 0),        draw(s, -0.7f, 1.0f, kCcw, kReversed), draw(s, 0.0f, -1.0f, kCw, 0),
      draw(s, 0.0f, -1.0f, kCw, kReversed), draw(s, 0.7f, 1.0f, kCcw, 0),          draw(s, 0.7f, 1.0f, kCcw, kReversed),
  };
  const auto frame = test::renderTriangles(s.vertices, draws);
  const auto n = countByThird(frame);
  INFO("left " << n[0] << " middle " << n[1] << " right " << n[2]);
  CHECK(n[0] > 100);
  CHECK(n[1] > 100);
  CHECK(n[2] > 100);
  // The golden triangle is symmetric about x = 0, so the mirror covers the same pixel count (±1 row of edge).
  CHECK(std::abs(n[1] - n[0]) <= 40);
  CHECK(frame.validationErrors == 0);

  // Only one triangle of each pair survives: the frame equals the one drawn without the reversed copies.
  const std::array frontOnly{draws[0], draws[2], draws[4]};
  CHECK(countByThird(test::renderTriangles(s.vertices, frontOnly)) == n);
}

TEST_CASE("control: a mirrored draw with the ordinary front face is culled", "[renderer][stage1]") {
  const Setup s = load();
  const std::array wrong{draw(s, 0.0f, -1.0f, kCcw, 0)};
  const auto frame = test::renderTriangles(s.vertices, wrong);
  CHECK(countByThird(frame) == std::array<int, 3>{0, 0, 0});
  CHECK(frame.validationErrors == 0);
}
