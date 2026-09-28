// ROADMAP Stage 1 acceptance: landmark colors and foreground coverage of the frozen triangle
// (tests/golden/triangle.json), checked against an oracle that does not use the GPU.

#include "golden_reader.hpp"
#include "triangle_fixture.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdlib>
#include <optional>

using namespace axiom;
using axiom::renderer::Rgba8;

namespace {

struct Golden {
  test::TriangleDraw draw{};
  std::array<renderer::TriangleVertex, 3> vertices{};
  std::vector<double> landmarks; // x0 y0 x1 y1 ...
  int tolerance = 0;
  double coverageTolerance = 0;
};

Golden loadGolden() {
  const std::string json = test::readGolden("triangle.json");
  Golden g;
  const auto mvp = test::goldenNumbers(json, "mvp_column_major");
  REQUIRE(mvp.size() == 16);
  for (int i = 0; i < 16; ++i) {
    g.draw.mvp[i] = static_cast<float>(mvp[i]);
  }
  g.draw.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  const auto v = test::goldenNumbers(json, "vertices");
  REQUIRE(v.size() == 18);
  for (int i = 0; i < 3; ++i) {
    for (int k = 0; k < 3; ++k) {
      g.vertices[i].position[k] = static_cast<float>(v[i * 6 + k]);
      g.vertices[i].color[k] = static_cast<float>(v[i * 6 + 3 + k]);
    }
  }
  g.landmarks = test::goldenNumbers(json, "landmarks");
  REQUIRE(g.landmarks.size() % 2 == 0);
  REQUIRE(test::goldenNumbers(json, "width") == std::vector<double>{256});
  g.tolerance = static_cast<int>(test::goldenNumbers(json, "channel_tolerance")[0]);
  g.coverageTolerance = test::goldenNumbers(json, "coverage_tolerance")[0];
  return g;
}

struct ScreenVertex {
  double x, y, w;
};

// Clip = MVP * (p, 1); NDC = clip / w; framebuffer = (ndc + 1) * size / 2 (viewport at 0,0, positive height).
std::array<ScreenVertex, 3> project(const Golden &g) {
  std::array<ScreenVertex, 3> s{};
  for (int i = 0; i < 3; ++i) {
    const double p[4] = {g.vertices[i].position[0], g.vertices[i].position[1], g.vertices[i].position[2], 1.0};
    double clip[4] = {0, 0, 0, 0};
    for (int r = 0; r < 4; ++r) {
      for (int c = 0; c < 4; ++c) {
        clip[r] += g.draw.mvp[c * 4 + r] * p[c];
      }
    }
    s[i] = {(clip[0] / clip[3] + 1.0) * test::kFrameSize / 2.0, (clip[1] / clip[3] + 1.0) * test::kFrameSize / 2.0,
            clip[3]};
  }
  return s;
}

double edge(const ScreenVertex &a, const ScreenVertex &b, double px, double py) {
  return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
}

int srgbByte(double linear) {
  const double c = linear <= 0.0031308 ? 12.92 * linear : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
  return static_cast<int>(std::lround(c * 255.0));
}

// Expected sRGB bytes at the center of pixel (x, y), or nullopt outside the triangle.
// perspective = false gives the affine (screen-linear) result, used only to show the test discriminates.
std::optional<std::array<int, 3>> oracle(const Golden &g, int x, int y, bool perspective = true) {
  const auto s = project(g);
  const double px = x + 0.5, py = y + 0.5;
  const double area = edge(s[0], s[1], s[2].x, s[2].y);
  const double b[3] = {edge(s[1], s[2], px, py) / area, edge(s[2], s[0], px, py) / area,
                       edge(s[0], s[1], px, py) / area};
  if (b[0] < 0 || b[1] < 0 || b[2] < 0) {
    return std::nullopt;
  }
  double q[3], sum = 0;
  for (int i = 0; i < 3; ++i) {
    q[i] = perspective ? b[i] / s[i].w : b[i];
    sum += q[i];
  }
  std::array<int, 3> out{};
  for (int k = 0; k < 3; ++k) {
    double c = 0;
    for (int i = 0; i < 3; ++i) {
      c += q[i] * g.vertices[i].color[k];
    }
    out[k] = srgbByte(c / sum);
  }
  return out;
}

} // namespace

TEST_CASE("triangle landmarks match the perspective-correct sRGB oracle", "[renderer][stage1]") {
  const Golden g = loadGolden();
  const auto frame = test::renderTriangles(g.vertices, std::span{&g.draw, 1});
  CHECK(frame.validationErrors == 0);

  bool discriminates = false;
  for (std::size_t i = 0; i < g.landmarks.size(); i += 2) {
    const int x = static_cast<int>(g.landmarks[i]), y = static_cast<int>(g.landmarks[i + 1]);
    const auto expected = oracle(g, x, y).value_or(std::array<int, 3>{0, 0, 0});
    const Rgba8 got = frame.pixels[static_cast<std::size_t>(y) * test::kFrameSize + x];
    INFO("landmark (" << x << ", " << y << ") expected " << expected[0] << ' ' << expected[1] << ' ' << expected[2]
                      << " got " << int(got.r) << ' ' << int(got.g) << ' ' << int(got.b) << ' ' << int(got.a));
    CHECK(std::abs(got.r - expected[0]) <= g.tolerance);
    CHECK(std::abs(got.g - expected[1]) <= g.tolerance);
    CHECK(std::abs(got.b - expected[2]) <= g.tolerance);
    CHECK(got.a == 255); // alpha is linear: opaque vertex output over an opaque clear
    if (const auto affine = oracle(g, x, y, false)) {
      for (int k = 0; k < 3; ++k) {
        discriminates = discriminates || std::abs((*affine)[k] - expected[k]) > g.tolerance;
      }
    }
  }
  CHECK(discriminates); // at least one landmark would fail if interpolation were affine
}

TEST_CASE("triangle foreground coverage is within 1% of the analytic area", "[renderer][stage1]") {
  const Golden g = loadGolden();
  const auto frame = test::renderTriangles(g.vertices, std::span{&g.draw, 1});
  CHECK(frame.validationErrors == 0);

  const auto s = project(g);
  const double analytic = std::abs(edge(s[0], s[1], s[2].x, s[2].y)) / 2.0;
  std::size_t foreground = 0;
  for (const Rgba8 &p : frame.pixels) {
    foreground += (p.r | p.g | p.b) != 0 ? 1 : 0;
  }
  INFO("foreground " << foreground << " analytic " << analytic);
  CHECK(std::abs(static_cast<double>(foreground) - analytic) <= g.coverageTolerance * analytic);
}
