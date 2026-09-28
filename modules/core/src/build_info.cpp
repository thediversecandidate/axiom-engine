#include "axiom/core/build_info.hpp"

#include <version> // defines __GLIBCXX__ under libstdc++

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

long engineLibstdcxxVersion() noexcept {
#if defined(__GLIBCXX__)
  return __GLIBCXX__;
#else
  return 0;
#endif
}

} // namespace axiom::core
