#pragma once
// Matrix4 — CONVENTIONS §3: column-major `float data[16]`, column vectors (v' = M·v), M_total = P·V·W;
// §4: alignas(16). Element (row r, column c) is data[c * 4 + r].

#include "axiom/math/vector.hpp"

#include <optional>

namespace axiom::math {

struct alignas(16) Matrix4 {
  float data[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; // identity

  [[nodiscard]] constexpr float at(int row, int column) const noexcept { return data[column * 4 + row]; }
  [[nodiscard]] constexpr Vector4 column(int c) const noexcept {
    return {data[c * 4], data[c * 4 + 1], data[c * 4 + 2], data[c * 4 + 3]};
  }

  [[nodiscard]] static constexpr Matrix4 identity() noexcept { return {}; }
  [[nodiscard]] static constexpr Matrix4 fromColumns(Vector4 c0, Vector4 c1, Vector4 c2, Vector4 c3) noexcept {
    return {{c0.x, c0.y, c0.z, c0.w, c1.x, c1.y, c1.z, c1.w, c2.x, c2.y, c2.z, c2.w, c3.x, c3.y, c3.z, c3.w}};
  }
  [[nodiscard]] static constexpr Matrix4 translation(Vector3 t) noexcept {
    return fromColumns({1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {t.x, t.y, t.z, 1});
  }
  [[nodiscard]] static constexpr Matrix4 scale(Vector3 s) noexcept {
    return fromColumns({s.x, 0, 0, 0}, {0, s.y, 0, 0}, {0, 0, s.z, 0}, {0, 0, 0, 1});
  }

  friend constexpr bool operator==(const Matrix4 &, const Matrix4 &) = default; // exact, for tests
};

static_assert(sizeof(Matrix4) == 64 && alignof(Matrix4) == 16, "CONVENTIONS §4: Matrix4 is alignas(16)");

/// a·b: applies b first, then a.
[[nodiscard]] constexpr Matrix4 operator*(const Matrix4 &a, const Matrix4 &b) noexcept {
  Matrix4 r;
  for (int c = 0; c < 4; ++c) {
    for (int row = 0; row < 4; ++row) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a.at(row, k) * b.at(k, c);
      }
      r.data[c * 4 + row] = sum;
    }
  }
  return r;
}

[[nodiscard]] constexpr Vector4 operator*(const Matrix4 &m, Vector4 v) noexcept {
  return {m.at(0, 0) * v.x + m.at(0, 1) * v.y + m.at(0, 2) * v.z + m.at(0, 3) * v.w,
          m.at(1, 0) * v.x + m.at(1, 1) * v.y + m.at(1, 2) * v.z + m.at(1, 3) * v.w,
          m.at(2, 0) * v.x + m.at(2, 1) * v.y + m.at(2, 2) * v.z + m.at(2, 3) * v.w,
          m.at(3, 0) * v.x + m.at(3, 1) * v.y + m.at(3, 2) * v.z + m.at(3, 3) * v.w};
}

/// M·(p, 1) without the perspective divide (for affine transforms).
[[nodiscard]] constexpr Vector3 transformPoint(const Matrix4 &m, Vector3 p) noexcept {
  return xyz(m * toVector4(p, 1));
}
/// M·(d, 0): translation does not apply.
[[nodiscard]] constexpr Vector3 transformDirection(const Matrix4 &m, Vector3 d) noexcept {
  return xyz(m * toVector4(d, 0));
}

[[nodiscard]] constexpr Matrix4 transpose(const Matrix4 &m) noexcept {
  Matrix4 r;
  for (int c = 0; c < 4; ++c) {
    for (int row = 0; row < 4; ++row) {
      r.data[c * 4 + row] = m.at(c, row);
    }
  }
  return r;
}

/// Computed in double precision, rounded to float.
[[nodiscard]] float determinant(const Matrix4 &m) noexcept;
/// Inverse computed in double precision. Empty if the input is non-finite, the determinant is exactly zero,
/// or the result is non-finite. Conditioning limits for world transforms (rcond∞, CONVENTIONS §4) are applied
/// by the transform validation, not here.
[[nodiscard]] std::optional<Matrix4> tryInverse(const Matrix4 &m) noexcept;
[[nodiscard]] bool isFinite(const Matrix4 &m) noexcept;
/// Element-wise approxEqual (absolute near zero, relative above magnitude 1).
[[nodiscard]] bool approxEqual(const Matrix4 &a, const Matrix4 &b, float tolerance = kDefaultTolerance) noexcept;

} // namespace axiom::math
