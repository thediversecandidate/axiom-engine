// Fixture: normalized() on a zero vector must abort in debug and asan-ubsan (math.normalized_asserts) and
// return the zero vector without aborting in release (math.normalized_release_no_abort).
#include "axiom/math/vector.hpp"

#include <cstdio>

int main() {
  const axiom::math::Vector3 n = axiom::math::normalized(axiom::math::Vector3{});
  std::printf("normalized returned (%g, %g, %g)\n", static_cast<double>(n.x), static_cast<double>(n.y),
              static_cast<double>(n.z));
  return (n.x == 0.0f && n.y == 0.0f && n.z == 0.0f) ? 0 : 1;
}
