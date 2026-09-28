#include "axiom/core/build_info.hpp"

#include <version> // defines _LIBCPP_VERSION under libc++

namespace axiom::core {

bool engineBuiltWithExceptions() noexcept {
#if defined(__cpp_exceptions)
  return true;
#else
  return false;
#endif
}

bool engineBuiltWithRtti() noexcept {
#if defined(__cpp_rtti)
  return true;
#else
  return false;
#endif
}

long engineLibcxxVersion() noexcept {
#if defined(_LIBCPP_VERSION)
  return _LIBCPP_VERSION;
#else
  return 0;
#endif
}

} // namespace axiom::core
