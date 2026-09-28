// ROADMAP Stage 1 acceptance: full-image comparison of the frozen triangle to the reviewed reference.
// At most 0.1% (65) pixels may differ by more than 2/255 in any channel. The actual image is always
// written to <build>/test_output/; on failure a diff image is written next to it (CI uploads both as PNG).

#include "image_compare.hpp"
#include "triangle_fixture.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace axiom;

namespace {

constexpr int kTolerance = 2;
constexpr std::size_t kMaxExceeding = 65; // 0.1% of 256 x 256, rounded down

const std::filesystem::path kOutputDir = AXIOM_TEST_OUTPUT_DIR;
const std::filesystem::path kReference = std::filesystem::path(AXIOM_GOLDEN_DIR) / "triangle.reference.pam";

test::Image renderGoldenTriangle(std::uint32_t &validationErrors) {
  const test::TriangleScene scene = test::loadTriangleScene(test::readGolden("triangle.json"));
  test::TriangleDraw draw{};
  std::copy(std::begin(scene.mvp), std::end(scene.mvp), draw.mvp);
  draw.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  const auto frame = test::renderTriangles(scene.vertices, std::span{&draw, 1});
  validationErrors = frame.validationErrors;
  return test::Image{test::kFrameSize, test::kFrameSize, frame.pixels};
}

} // namespace

TEST_CASE("the triangle matches the reviewed reference image", "[renderer][stage1]") {
  std::uint32_t validationErrors = 0;
  const test::Image actual = renderGoldenTriangle(validationErrors);
  CHECK(validationErrors == 0);
  REQUIRE(test::writePam(kOutputDir / "triangle.actual.pam", actual));

  const auto reference = test::readPam(kReference);
  INFO("reference " << kReference.string() << " (missing or malformed references are never created by tests;"
                    << " see tests/golden/README.md)");
  REQUIRE(reference.has_value());
  REQUIRE(reference->width == actual.width);
  REQUIRE(reference->height == actual.height);

  const auto result = test::compareImages(actual, *reference, kTolerance);
  INFO(result.exceeding << " pixels differ by more than " << kTolerance << "/255; max delta " << result.maxDelta);
  if (result.exceeding > kMaxExceeding) {
    CHECK(test::writePam(kOutputDir / "triangle.diff.pam", result.diff));
  }
  CHECK(result.exceeding <= kMaxExceeding);
}

TEST_CASE("the image comparison enforces the 65-pixel limit exactly", "[renderer][stage1]") {
  test::Image reference{256, 256, std::vector<renderer::Rgba8>(256 * 256, renderer::Rgba8{10, 20, 30, 255})};
  test::Image actual = reference;
  for (std::size_t i = 0; i < 65; ++i) {
    actual.pixels[i * 997].g = 23; // delta 3 > tolerance
  }
  actual.pixels[1].b = 32; // delta 2: within tolerance, not counted
  const auto at = test::compareImages(actual, reference, kTolerance);
  CHECK(at.exceeding == 65);
  CHECK(at.maxDelta == 3);
  actual.pixels[2].a = 251;
  CHECK(test::compareImages(actual, reference, kTolerance).exceeding == 66);
}

TEST_CASE("PAM images round-trip and malformed files are rejected", "[renderer][stage1]") {
  test::Image img{
      3, 2, {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}, {17, 18, 19, 20}, {0, 0, 0, 255}}};
  const auto path = kOutputDir / "roundtrip.pam";
  REQUIRE(test::writePam(path, img));
  const auto back = test::readPam(path);
  REQUIRE(back.has_value());
  CHECK(back->width == 3);
  CHECK(back->height == 2);
  CHECK(back->pixels[4].r == 17);
  CHECK_FALSE(test::readPam(kOutputDir / "does_not_exist.pam").has_value());
  std::filesystem::resize_file(path, std::filesystem::file_size(path) - 1); // truncated payload
  CHECK_FALSE(test::readPam(path).has_value());
}
