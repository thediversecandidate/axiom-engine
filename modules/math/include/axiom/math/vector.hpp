#pragma once
// Vector3 (12 bytes) and Vector4 (alignas(16)) — CONVENTIONS §3 (right-handed, SI units) and §4 (CPU types).
// Scalar reference implementations; AVX2 paths arrive in a later Stage 2 step and must equal these.

#include "axiom/math/scalar.hpp"

#include <optional>

namespace axiom::math {

struct Vector3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  friend constexpr bool operator==(const Vector3 &, const Vector3 &) = default; // exact, for tests
};

struct alignas(16) Vector4 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 0.0f;

  friend constexpr bool operator==(const Vector4 &, const Vector4 &) = default; // exact, for tests
};

static_assert(sizeof(Vector3) == 12, "CONVENTIONS §4: Vector3 is 12 bytes");
static_assert(sizeof(Vector4) == 16 && alignof(Vector4) == 16, "CONVENTIONS §4: Vector4 is alignas(16)");

// --- Vector3 ---
[[nodiscard]] constexpr Vector3 operator+(Vector3 a, Vector3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
[[nodiscard]] constexpr Vector3 operator-(Vector3 a, Vector3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
[[nodiscard]] constexpr Vector3 operator-(Vector3 a) noexcept { return {-a.x, -a.y, -a.z}; }
[[nodiscard]] constexpr Vector3 operator*(Vector3 a, float s) noexcept { return {a.x * s, a.y * s, a.z * s}; }
[[nodiscard]] constexpr Vector3 operator*(float s, Vector3 a) noexcept { return a * s; }
[[nodiscard]] constexpr float dot(Vector3 a, Vector3 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
/// Right-handed: cross(+X, +Y) = +Z.
[[nodiscard]] constexpr Vector3 cross(Vector3 a, Vector3 b) noexcept {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
[[nodiscard]] bool isFinite(Vector3 v) noexcept;
/// Overflow-safe (scaled by the largest component): finite whenever the true length fits in a float.
[[nodiscard]] float length(Vector3 v) noexcept;
/// Empty for non-finite input or length < 1e-12 (CONVENTIONS §3); otherwise unit length within 1e-6.
[[nodiscard]] std::optional<Vector3> tryNormalize(Vector3 v) noexcept;
/// Requires a finite input with length >= 1e-12 (asserted in debug and asan-ubsan).
[[nodiscard]] Vector3 normalized(Vector3 v) noexcept;
/// Component-wise approxEqual (absolute near zero, relative above magnitude 1).
[[nodiscard]] bool approxEqual(Vector3 a, Vector3 b, float tolerance = kDefaultTolerance) noexcept;

// --- Vector4 ---
[[nodiscard]] constexpr Vector4 operator+(Vector4 a, Vector4 b) noexcept {
  return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
[[nodiscard]] constexpr Vector4 operator-(Vector4 a, Vector4 b) noexcept {
  return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}
[[nodiscard]] constexpr Vector4 operator-(Vector4 a) noexcept { return {-a.x, -a.y, -a.z, -a.w}; }
[[nodiscard]] constexpr Vector4 operator*(Vector4 a, float s) noexcept { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
[[nodiscard]] constexpr Vector4 operator*(float s, Vector4 a) noexcept { return a * s; }
[[nodiscard]] constexpr float dot(Vector4 a, Vector4 b) noexcept {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
[[nodiscard]] constexpr Vector4 toVector4(Vector3 v, float w) noexcept { return {v.x, v.y, v.z, w}; }
[[nodiscard]] constexpr Vector3 xyz(Vector4 v) noexcept { return {v.x, v.y, v.z}; }
[[nodiscard]] bool isFinite(Vector4 v) noexcept;
[[nodiscard]] float length(Vector4 v) noexcept;
[[nodiscard]] std::optional<Vector4> tryNormalize(Vector4 v) noexcept;
[[nodiscard]] Vector4 normalized(Vector4 v) noexcept;
[[nodiscard]] bool approxEqual(Vector4 a, Vector4 b, float tolerance = kDefaultTolerance) noexcept;

} // namespace axiom::math
