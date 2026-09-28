#include "axiom/core/assert.hpp"

#include <cstdio>
#include <cstdlib>

namespace axiom::core {

void assertFailed(const char *expression, const char *message, const char *file, int line) noexcept {
  std::fprintf(stderr, "AXIOM_ASSERT failed: %s\n  message: %s\n  at %s:%d\n", expression, message, file, line);
  std::fflush(stderr);
  std::abort();
}

} // namespace axiom::core
