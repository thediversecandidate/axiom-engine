#include "axiom/math/vector.hpp"

#include "math_assert.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>

namespace axiom::math {

namespace detail {
void assertFailed(const char *expression, const char *message, const char *file, int line) noexcept {
  std::fprintf(stderr, "AXIOM_MATH_ASSERT failed: %s (%s) at %s:%d\n", expression, message, file, line);
  std::abort();
}
} // namespace detail

namespace {

constexpr float kMinLength = 1e-12f; // CONVENTIONS §3

// Largest absolute component; the scale that keeps squared terms in range for any finite input.
template <std::size_t N> float maxAbs(const float (&c)[N]) {
  float m = 0.0f;
  for (float v : c) {
    m = std::max(m, std::fabs(v));
  }
  return m;
}

template <std::size_t N> bool finite(const float (&c)[N]) {
  return std::all_of(std::begin(c), std::end(c), [](float v) { return std::isfinite(v); });
}

// Length as m * |c / m| (overflow- and underflow-safe); 0 for the zero vector.
template <std::size_t N> float scaledLength(const float (&c)[N]) {
  const float m = maxAbs(c);
  if (m == 0.0f) {
    return 0.0f;
  }
  float sum = 0.0f;
  for (float v : c) {
    const float u = v / m;
    sum += u * u;
  }
  return m * std::sqrt(sum);
}

// Writes c / |c| into out; false for non-finite input or length < kMinLength.
template <std::size_t N> bool normalizeInto(const float (&c)[N], float (&out)[N]) {
  if (!finite(c)) {
    return false;
  }
  const float m = maxAbs(c);
  if (m == 0.0f || scaledLength(c) < kMinLength) {
    return false;
  }
  float sum = 0.0f;
  for (std::size_t i = 0; i < N; ++i) {
    out[i] = c[i] / m;
    sum += out[i] * out[i];
  }
  const float inv = 1.0f / std::sqrt(sum);
  for (float &v : out) {
    v *= inv;
  }
  return true;
}

} // namespace

bool isFinite(Vector3 v) noexcept { return finite({v.x, v.y, v.z}); }
bool isFinite(Vector4 v) noexcept { return finite({v.x, v.y, v.z, v.w}); }
float length(Vector3 v) noexcept { return scaledLength({v.x, v.y, v.z}); }
float length(Vector4 v) noexcept { return scaledLength({v.x, v.y, v.z, v.w}); }

std::optional<Vector3> tryNormalize(Vector3 v) noexcept {
  float out[3];
  if (!normalizeInto({v.x, v.y, v.z}, out)) {
    return std::nullopt;
  }
  return Vector3{out[0], out[1], out[2]};
}

std::optional<Vector4> tryNormalize(Vector4 v) noexcept {
  float out[4];
  if (!normalizeInto({v.x, v.y, v.z, v.w}, out)) {
    return std::nullopt;
  }
  return Vector4{out[0], out[1], out[2], out[3]};
}

Vector3 normalized(Vector3 v) noexcept {
  const auto n = tryNormalize(v);
  AXIOM_MATH_ASSERT(n.has_value(), "normalized() needs a finite vector with length >= 1e-12");
  return n.value_or(Vector3{});
}

Vector4 normalized(Vector4 v) noexcept {
  const auto n = tryNormalize(v);
  AXIOM_MATH_ASSERT(n.has_value(), "normalized() needs a finite vector with length >= 1e-12");
  return n.value_or(Vector4{});
}

bool approxEqual(Vector3 a, Vector3 b, float tolerance) noexcept {
  return approxEqual(a.x, b.x, tolerance) && approxEqual(a.y, b.y, tolerance) && approxEqual(a.z, b.z, tolerance);
}

bool approxEqual(Vector4 a, Vector4 b, float tolerance) noexcept {
  return approxEqual(a.x, b.x, tolerance) && approxEqual(a.y, b.y, tolerance) && approxEqual(a.z, b.z, tolerance) &&
         approxEqual(a.w, b.w, tolerance);
}

} // namespace axiom::math
