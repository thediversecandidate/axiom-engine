# Module `math` — public API
Depends on: none

## `modules/math/include/axiom/math/matrix.hpp` — Matrix4 — CONVENTIONS §3: column-major `float data[16]`, column vectors (v' = M·v), M_total = P·V·W;
- `struct alignas(16) Matrix4`
  - `float data[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}`
  - `[[nodiscard]] constexpr float at(int row, int column) const noexcept`
  - `[[nodiscard]] constexpr Vector4 column(int c) const noexcept`
  - `[[nodiscard]] static constexpr Matrix4 identity() noexcept`
  - `[[nodiscard]] static constexpr Matrix4 fromColumns(Vector4 c0, Vector4 c1, Vector4 c2, Vector4 c3) noexcept`
  - `[[nodiscard]] static constexpr Matrix4 translation(Vector3 t) noexcept`
  - `[[nodiscard]] static constexpr Matrix4 scale(Vector3 s) noexcept`
- `[[nodiscard]] constexpr Matrix4 operator*(const Matrix4 &a, const Matrix4 &b) noexcept`
  /// a·b: applies b first, then a.
- `[[nodiscard]] constexpr Vector4 operator*(const Matrix4 &m, Vector4 v) noexcept`
- `[[nodiscard]] constexpr Vector3 transformPoint(const Matrix4 &m, Vector3 p) noexcept`
  /// M·(p, 1) without the perspective divide (for affine transforms).
- `[[nodiscard]] constexpr Vector3 transformDirection(const Matrix4 &m, Vector3 d) noexcept`
  /// M·(d, 0): translation does not apply.
- `[[nodiscard]] constexpr Matrix4 transpose(const Matrix4 &m) noexcept`
- `[[nodiscard]] float determinant(const Matrix4 &m) noexcept`
  /// Computed in double precision, rounded to float.
- `[[nodiscard]] std::optional<Matrix4> tryInverse(const Matrix4 &m) noexcept`
  /// Inverse computed in double precision. Empty if the input is non-finite, the determinant is exactly zero,
  /// or the result is non-finite. Conditioning limits for world transforms (rcond∞, CONVENTIONS §4) are applied
  /// by the transform validation, not here.
- `[[nodiscard]] bool isFinite(const Matrix4 &m) noexcept`
- `[[nodiscard]] bool approxEqual(const Matrix4 &a, const Matrix4 &b, float tolerance = kDefaultTolerance) noexcept`
  /// Element-wise approxEqual (absolute near zero, relative above magnitude 1).

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
