# Module `math` — public API
Depends on: none

## `modules/math/include/axiom/math/scalar.hpp` — Scalar helpers. The math module depends on nothing (CONTEXT_RULES §1); fallible math APIs
- `inline constexpr float kPi = 3.14159265358979323846f`
- `inline constexpr float kTwoPi = 2.0f * kPi`
- `inline constexpr float kDefaultTolerance = 1e-5f`
- `[[nodiscard]] bool approxEqual(float a, float b, float tolerance = kDefaultTolerance) noexcept`
  /// Absolute tolerance for |a| <= 1, relative tolerance above magnitude 1 (CONVENTIONS §3).
- `[[nodiscard]] std::optional<float> wrapAngle(float radians) noexcept`
  /// Wraps an angle in radians into [-pi, pi]. Empty for non-finite input.
