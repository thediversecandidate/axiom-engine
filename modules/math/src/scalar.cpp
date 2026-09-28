#include "axiom/math/scalar.hpp"

#include <algorithm>

namespace axiom::math {

bool approxEqual(float a, float b, float tolerance) noexcept {
  const float scale = std::max({1.0f, std::fabs(a), std::fabs(b)});
  return std::fabs(a - b) <= tolerance * scale;
}

std::optional<float> wrapAngle(float radians) noexcept {
  if (!std::isfinite(radians)) {
    return std::nullopt;
  }
  float wrapped = std::remainder(radians, kTwoPi);
  if (wrapped < -kPi) {
    wrapped += kTwoPi;
  }
  return wrapped;
}

} // namespace axiom::math
