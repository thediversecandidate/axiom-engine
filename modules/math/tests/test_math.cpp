#include "axiom/math/scalar.hpp"

#include <catch2/catch_test_macros.hpp>
#include <limits>

using axiom::math::approxEqual;
using axiom::math::kPi;
using axiom::math::wrapAngle;

TEST_CASE("approxEqual uses absolute tolerance near zero and relative above one", "[math]") {
  CHECK(approxEqual(0.0f, 5e-6f));
  CHECK_FALSE(approxEqual(0.0f, 2e-5f));
  CHECK(approxEqual(1000.0f, 1000.005f));
  CHECK_FALSE(approxEqual(1000.0f, 1000.05f));
}

TEST_CASE("wrapAngle maps into [-pi, pi] and rejects non-finite input", "[math]") {
  const auto wrapped = wrapAngle(3.0f * kPi);
  REQUIRE(wrapped.has_value());
  CHECK(approxEqual(std::fabs(*wrapped), kPi));
  CHECK_FALSE(wrapAngle(std::numeric_limits<float>::quiet_NaN()).has_value());
  CHECK_FALSE(wrapAngle(std::numeric_limits<float>::infinity()).has_value());
}
