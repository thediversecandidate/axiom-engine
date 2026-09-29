#pragma once
// Private to math: the equivalent of AXIOM_ASSERT (CONVENTIONS §2) without depending on core
// (CONTEXT_RULES §1). Active in debug and asan-ubsan, compiled out in release (NDEBUG).

namespace axiom::math::detail {
[[noreturn]] void assertFailed(const char *expression, const char *message, const char *file, int line) noexcept;
} // namespace axiom::math::detail

#if defined(NDEBUG)
#define AXIOM_MATH_ASSERT(cond, msg) ((void)0)
#else
#define AXIOM_MATH_ASSERT(cond, msg)                                                                                   \
  ((cond) ? (void)0 : ::axiom::math::detail::assertFailed(#cond, (msg), __FILE__, __LINE__))
#endif
