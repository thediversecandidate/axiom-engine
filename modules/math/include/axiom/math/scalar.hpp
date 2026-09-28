#pragma once
// Scalar helpers. The math module depends on nothing (CONTEXT_RULES §1); fallible math APIs
// return std::optional, never core::Result (CONVENTIONS §2).

#include <cmath>
#include <optional>

namespace axiom::math {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 2.0f * kPi;
inline constexpr float kDefaultTolerance = 1e-5f; // CONVENTIONS §3

/// Absolute tolerance for |a| <= 1, relative tolerance above magnitude 1 (CONVENTIONS §3).
[[nodiscard]] bool approxEqual(float a, float b, float tolerance = kDefaultTolerance) noexcept;

/// Wraps an angle in radians into [-pi, pi]. Empty for non-finite input.
[[nodiscard]] std::optional<float> wrapAngle(float radians) noexcept;

} // namespace axiom::math
