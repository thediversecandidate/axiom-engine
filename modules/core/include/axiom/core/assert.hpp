#pragma once
// AXIOM_ASSERT: active in debug and asan-ubsan, compiled out in release (CONVENTIONS §2).

namespace axiom::core {

/// Logs file/line/message to stderr, then calls std::abort(). Never returns.
[[noreturn]] void assertFailed(const char *expression, const char *message, const char *file, int line) noexcept;

} // namespace axiom::core

#if defined(NDEBUG)
#define AXIOM_ASSERT(cond, msg) ((void)0)
#else
#define AXIOM_ASSERT(cond, msg) ((cond) ? (void)0 : ::axiom::core::assertFailed(#cond, (msg), __FILE__, __LINE__))
#endif
