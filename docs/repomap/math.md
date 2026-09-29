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

## `modules/math/include/axiom/math/vector.hpp` — Vector3 (12 bytes) and Vector4 (alignas(16)) — CONVENTIONS §3 (right-handed, SI units) and §4 (CPU types).
- `struct Vector3`
  - `float x = 0.0f`
  - `float y = 0.0f`
  - `float z = 0.0f`
- `struct alignas(16) Vector4`
  - `float x = 0.0f`
  - `float y = 0.0f`
  - `float z = 0.0f`
  - `float w = 0.0f`
- `[[nodiscard]] constexpr Vector3 operator+(Vector3 a, Vector3 b) noexcept`
- `[[nodiscard]] constexpr Vector3 operator-(Vector3 a, Vector3 b) noexcept`
- `[[nodiscard]] constexpr Vector3 operator-(Vector3 a) noexcept`
- `[[nodiscard]] constexpr Vector3 operator*(Vector3 a, float s) noexcept`
- `[[nodiscard]] constexpr Vector3 operator*(float s, Vector3 a) noexcept`
- `[[nodiscard]] constexpr float dot(Vector3 a, Vector3 b) noexcept`
- `[[nodiscard]] constexpr Vector3 cross(Vector3 a, Vector3 b) noexcept`
  /// Right-handed: cross(+X, +Y) = +Z.
- `[[nodiscard]] bool isFinite(Vector3 v) noexcept`
- `[[nodiscard]] float length(Vector3 v) noexcept`
  /// Overflow-safe (scaled by the largest component): finite whenever the true length fits in a float.
- `[[nodiscard]] std::optional<Vector3> tryNormalize(Vector3 v) noexcept`
  /// Empty for non-finite input or length < 1e-12 (CONVENTIONS §3); otherwise unit length within 1e-6.
- `[[nodiscard]] Vector3 normalized(Vector3 v) noexcept`
  /// Requires a finite input with length >= 1e-12 (asserted in debug and asan-ubsan).
- `[[nodiscard]] bool approxEqual(Vector3 a, Vector3 b, float tolerance = kDefaultTolerance) noexcept`
  /// Component-wise approxEqual (absolute near zero, relative above magnitude 1).
- `[[nodiscard]] constexpr Vector4 operator+(Vector4 a, Vector4 b) noexcept`
- `[[nodiscard]] constexpr Vector4 operator-(Vector4 a, Vector4 b) noexcept`
- `[[nodiscard]] constexpr Vector4 operator-(Vector4 a) noexcept`
- `[[nodiscard]] constexpr Vector4 operator*(Vector4 a, float s) noexcept`
- `[[nodiscard]] constexpr Vector4 operator*(float s, Vector4 a) noexcept`
- `[[nodiscard]] constexpr float dot(Vector4 a, Vector4 b) noexcept`
- `[[nodiscard]] constexpr Vector4 toVector4(Vector3 v, float w) noexcept`
- `[[nodiscard]] constexpr Vector3 xyz(Vector4 v) noexcept`
- `[[nodiscard]] bool isFinite(Vector4 v) noexcept`
- `[[nodiscard]] float length(Vector4 v) noexcept`
- `[[nodiscard]] std::optional<Vector4> tryNormalize(Vector4 v) noexcept`
- `[[nodiscard]] Vector4 normalized(Vector4 v) noexcept`
- `[[nodiscard]] bool approxEqual(Vector4 a, Vector4 b, float tolerance = kDefaultTolerance) noexcept`
