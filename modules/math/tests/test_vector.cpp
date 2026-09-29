// Stage 2 step 2a: Vector3 / Vector4 (CONVENTIONS §3 tolerances and normalization, §4 CPU types).

#include "axiom/math/vector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <random>

using namespace axiom::math;

namespace {
constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
constexpr float kInf = std::numeric_limits<float>::infinity();
constexpr std::uint32_t kSeed = 20260929; // fixed, logged seed (CONVENTIONS §3)
} // namespace

TEST_CASE("Vector3/Vector4 layout matches CONVENTIONS §4", "[math][stage2]") {
  STATIC_CHECK(sizeof(Vector3) == 12);
  STATIC_CHECK(sizeof(Vector4) == 16);
  STATIC_CHECK(alignof(Vector4) == 16);
  Vector4 packed[2];
  CHECK(reinterpret_cast<std::uintptr_t>(&packed[1]) % 16 == 0);
}

TEST_CASE("cross product is right-handed and anticommutative", "[math][stage2]") {
  const Vector3 x{1, 0, 0}, y{0, 1, 0}, z{0, 0, 1};
  STATIC_CHECK(cross(Vector3{1, 0, 0}, Vector3{0, 1, 0}) == Vector3{0, 0, 1});
  CHECK(cross(y, z) == x);
  CHECK(cross(z, x) == y);
  CHECK(cross(y, x) == -z);
  const Vector3 a{1.5f, -2.0f, 0.25f}, b{-0.5f, 3.0f, 4.0f};
  CHECK(approxEqual(cross(a, b), -cross(b, a)));
  CHECK(approxEqual(dot(cross(a, b), a), 0.0f));
  CHECK(approxEqual(dot(cross(a, b), b), 0.0f));
}

TEST_CASE("arithmetic and dot follow their definitions", "[math][stage2]") {
  const Vector3 a{1, 2, 3}, b{4, -5, 6};
  CHECK(a + b == Vector3{5, -3, 9});
  CHECK(a - b == Vector3{-3, 7, -3});
  CHECK(2.0f * a == Vector3{2, 4, 6});
  CHECK(dot(a, b) == 12.0f); // 4 - 10 + 18
  const Vector4 c{1, 2, 3, 4}, d{-1, 0.5f, 2, -3};
  CHECK(c + d == Vector4{0, 2.5f, 5, 1});
  CHECK(dot(c, d) == -6.0f); // -1 + 1 + 6 - 12
  CHECK(xyz(toVector4(a, 7)) == a);
  CHECK(toVector4(a, 7).w == 7.0f);
}

TEST_CASE("length is exact for known values and does not overflow in intermediate steps", "[math][stage2]") {
  CHECK(length(Vector3{3, 4, 0}) == 5.0f);
  CHECK(length(Vector4{1, 1, 1, 1}) == 2.0f);
  CHECK(length(Vector3{}) == 0.0f);
  const float big = length(Vector3{1e30f, 1e30f, 0});
  CHECK(std::isfinite(big));
  CHECK(approxEqual(big / 1e30f, std::sqrt(2.0f)));
  const float tiny = length(Vector3{1e-30f, 1e-30f, 1e-30f});
  CHECK(approxEqual(tiny / 1e-30f, std::sqrt(3.0f)));
  // True length 3e38 fits in a float although every squared component (2.25e76) does not.
  const float nearMax = length(Vector4{1.5e38f, 1.5e38f, 1.5e38f, 1.5e38f});
  CHECK(std::isfinite(nearMax));
  CHECK(approxEqual(nearMax / 3e38f, 1.0f));
}

TEST_CASE("tryNormalize is empty for zero-length, tiny and non-finite input", "[math][stage2]") {
  CHECK_FALSE(tryNormalize(Vector3{}).has_value());
  CHECK_FALSE(tryNormalize(Vector3{1e-13f, 0, 0}).has_value());
  CHECK_FALSE(tryNormalize(Vector3{kNaN, 0, 1}).has_value());
  CHECK_FALSE(tryNormalize(Vector3{kInf, 0, 0}).has_value());
  CHECK_FALSE(tryNormalize(Vector4{0, 0, 0, kNaN}).has_value());
  CHECK_FALSE(tryNormalize(Vector4{}).has_value());
  CHECK_FALSE(isFinite(Vector3{0, -kInf, 0}));
  CHECK(isFinite(Vector4{1, 2, 3, 4}));
}

TEST_CASE("tryNormalize returns the unit vector for known and extreme input", "[math][stage2]") {
  const auto a = tryNormalize(Vector3{3, 4, 0});
  REQUIRE(a.has_value());
  CHECK(approxEqual(*a, Vector3{0.6f, 0.8f, 0}));
  const auto small = tryNormalize(Vector3{1e-10f, 0, 0}); // above the 1e-12 threshold
  REQUIRE(small.has_value());
  CHECK(*small == Vector3{1, 0, 0});
  const auto big = tryNormalize(Vector3{1e30f, -1e30f, 0}); // squared components would overflow
  REQUIRE(big.has_value());
  CHECK(approxEqual(*big, Vector3{std::sqrt(0.5f), -std::sqrt(0.5f), 0}));
  const auto w = tryNormalize(Vector4{0, 0, 0, -2});
  REQUIRE(w.has_value());
  CHECK(*w == Vector4{0, 0, 0, -1});
  CHECK(normalized(Vector3{0, 0, 5}) == Vector3{0, 0, 1});
}

TEST_CASE("normalization of 10,000 seeded random vectors is unit length and parallel", "[math][stage2]") {
  INFO("seed " << kSeed);
  std::mt19937 rng(kSeed);
  std::uniform_real_distribution<float> component(-1e3f, 1e3f);
  for (int i = 0; i < 10000; ++i) {
    const Vector3 v{component(rng), component(rng), component(rng)};
    const auto n = tryNormalize(v);
    REQUIRE(n.has_value());
    REQUIRE(std::fabs(length(*n) - 1.0f) <= 1e-6f);
    REQUIRE(length(cross(*n, v)) <= 1e-5f * length(v)); // parallel
    REQUIRE(dot(*n, v) > 0.0f);                         // same direction
  }
}
