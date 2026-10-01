// Stage 2 step 2b: Matrix4 (CONVENTIONS §3 column-major, column vectors; §4 alignment and projection).

#include "axiom/math/matrix.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <random>

using namespace axiom::math;

namespace {

constexpr std::uint32_t kSeed = 20260930; // fixed, logged seed (CONVENTIONS §3)

// Diagonally dominant random matrix: entries in [-1, 1] plus 4 on the diagonal (well conditioned).
Matrix4 randomMatrix(std::mt19937 &rng) {
  std::uniform_real_distribution<float> u(-1.0f, 1.0f);
  Matrix4 m;
  for (int i = 0; i < 16; ++i) {
    m.data[i] = u(rng) + (i % 5 == 0 ? 4.0f : 0.0f);
  }
  return m;
}

} // namespace

TEST_CASE("Matrix4 storage is column-major with the translation in the last column", "[math][stage2]") {
  STATIC_CHECK(sizeof(Matrix4) == 64);
  STATIC_CHECK(alignof(Matrix4) == 16);
  constexpr Matrix4 t = Matrix4::translation({1, 2, 3});
  STATIC_CHECK(t.data[12] == 1.0f);
  STATIC_CHECK(t.data[13] == 2.0f);
  STATIC_CHECK(t.data[14] == 3.0f);
  STATIC_CHECK(t.at(0, 3) == 1.0f);
  STATIC_CHECK(Matrix4{} == Matrix4::identity());
  CHECK(t.column(3) == Vector4{1, 2, 3, 1});
}

TEST_CASE("points translate, directions do not", "[math][stage2]") {
  const Matrix4 t = Matrix4::translation({1, 2, 3});
  CHECK(transformPoint(t, {0, 0, 0}) == Vector3{1, 2, 3});
  CHECK(transformPoint(t, {1, 1, 1}) == Vector3{2, 3, 4});
  CHECK(transformDirection(t, {1, 1, 1}) == Vector3{1, 1, 1});
  const Matrix4 s = Matrix4::scale({2, 3, 4});
  CHECK(transformPoint(s, {1, 1, 1}) == Vector3{2, 3, 4});
  CHECK(Matrix4::identity() * Vector4{1, 2, 3, 4} == Vector4{1, 2, 3, 4});
}

TEST_CASE("a·b applies b first (hand-computed)", "[math][stage2]") {
  const Matrix4 t = Matrix4::translation({1, 0, 0});
  const Matrix4 s = Matrix4::scale({2, 2, 2});
  // t·s: scale then translate: (1,0,0) -> (2,0,0) -> (3,0,0). s·t: translate then scale: -> (2,0,0) -> (4,0,0).
  CHECK(transformPoint(t * s, {1, 0, 0}) == Vector3{3, 0, 0});
  CHECK(transformPoint(s * t, {1, 0, 0}) == Vector3{4, 0, 0});
  CHECK((t * s).column(3) == Vector4{1, 0, 0, 1});
  CHECK((s * t).column(3) == Vector4{2, 0, 0, 1});
}

TEST_CASE("the CONVENTIONS §4 reversed-Z projection maps the near plane to 1 and far depths toward 0",
          "[math][stage2]") {
  const float f = 1.0f / std::tan(0.5f * kPi / 3.0f), aspect = 1.5f, n = 0.1f;
  const Matrix4 p = Matrix4::fromColumns({f / aspect, 0, 0, 0}, {0, -f, 0, 0}, {0, 0, 0, -1}, {0, 0, n, 0});
  const Vector4 nearClip = p * Vector4{0, 0, -n, 1};
  CHECK(approxEqual(nearClip.z / nearClip.w, 1.0f));
  const Vector4 farClip = p * Vector4{0, 0, -1e4f, 1};
  CHECK(approxEqual(farClip.z / farClip.w, n / 1e4f));
  const Vector4 up = p * Vector4{0, 1, -1, 1}; // +Y up in view space -> negative NDC y (the one Y inversion)
  CHECK(up.y / up.w < 0.0f);
}

TEST_CASE("determinant: known values and properties", "[math][stage2]") {
  CHECK(determinant(Matrix4::identity()) == 1.0f);
  CHECK(determinant(Matrix4::scale({2, 3, 4})) == 24.0f);
  CHECK(determinant(Matrix4::scale({-1, 1, 1})) == -1.0f); // mirror
  CHECK(determinant(Matrix4::translation({5, -7, 9})) == 1.0f);
  const Matrix4 swapXY = Matrix4::fromColumns({0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1});
  CHECK(determinant(swapXY) == -1.0f);
  CHECK(determinant(Matrix4::scale({1, 0, 1})) == 0.0f);
  INFO("seed " << kSeed);
  std::mt19937 rng(kSeed);
  for (int i = 0; i < 1000; ++i) {
    const Matrix4 a = randomMatrix(rng), b = randomMatrix(rng);
    REQUIRE(approxEqual(determinant(a * b) / (determinant(a) * determinant(b)), 1.0f, 1e-4f));
    REQUIRE(approxEqual(determinant(transpose(a)) / determinant(a), 1.0f, 1e-5f));
  }
}

TEST_CASE("tryInverse: hand-computed inverses and rejected input", "[math][stage2]") {
  const auto t = tryInverse(Matrix4::translation({1, 2, 3}));
  REQUIRE(t.has_value());
  CHECK(*t == Matrix4::translation({-1, -2, -3}));
  const auto s = tryInverse(Matrix4::scale({2, 4, 8}));
  REQUIRE(s.has_value());
  CHECK(*s == Matrix4::scale({0.5f, 0.25f, 0.125f}));
  CHECK_FALSE(tryInverse(Matrix4::scale({1, 0, 1})).has_value()); // singular
  Matrix4 nan;
  nan.data[6] = std::numeric_limits<float>::quiet_NaN();
  CHECK_FALSE(tryInverse(nan).has_value());
  Matrix4 inf;
  inf.data[1] = std::numeric_limits<float>::infinity();
  CHECK_FALSE(tryInverse(inf).has_value());
  // No absolute determinant cutoff: a tiny uniform scale inverts (1e30 fits in a float) ...
  const auto tiny = tryInverse(Matrix4::scale({1e-30f, 1e-30f, 1e-30f}));
  REQUIRE(tiny.has_value());
  CHECK(approxEqual(tiny->at(0, 0) / 1e30f, 1.0f));
  // ... but an inverse that overflows float (1e39) is rejected.
  CHECK_FALSE(tryInverse(Matrix4::scale({1e-39f, 1e-39f, 1e-39f})).has_value());
}

TEST_CASE("10,000 seeded random matrices: M·M⁻¹ = I, associativity, transpose, (AB)⁻¹ = B⁻¹A⁻¹", "[math][stage2]") {
  INFO("seed " << kSeed);
  std::mt19937 rng(kSeed);
  std::uniform_real_distribution<float> u(-10.0f, 10.0f);
  for (int i = 0; i < 10000; ++i) {
    const Matrix4 a = randomMatrix(rng), b = randomMatrix(rng);
    const auto ai = tryInverse(a);
    REQUIRE(ai.has_value());
    REQUIRE(approxEqual(a * *ai, Matrix4::identity()));
    REQUIRE(approxEqual(*ai * a, Matrix4::identity()));
    REQUIRE(transpose(transpose(a)) == a);
    const Vector4 v{u(rng), u(rng), u(rng), u(rng)};
    REQUIRE(approxEqual((a * b) * v, a * (b * v), 1e-4f));
    const auto abi = tryInverse(a * b), bi = tryInverse(b);
    REQUIRE((abi.has_value() && bi.has_value()));
    REQUIRE(approxEqual(*abi, *bi * *ai, 1e-5f));
  }
}
