# Module `core` — public API
Depends on: none

## `modules/core/include/axiom/core/assert.hpp` — AXIOM_ASSERT: active in debug and asan-ubsan, compiled out in release (CONVENTIONS §2).
- `[[noreturn]] void assertFailed(const char *expression, const char *message, const char *file, int line) noexcept`
  /// Logs file/line/message to stderr, then calls std::abort(). Never returns.
- `#define AXIOM_ASSERT(cond, msg) ((void)0)`
- `#define AXIOM_ASSERT(cond, msg) ((cond) ? (void)0 : ::axiom::core::assertFailed(#cond, (msg), __FILE__, __LINE__))`

## `modules/core/include/axiom/core/build_info.hpp` — Facts about how the engine library itself was compiled. Stage 0 tests use these to prove
- `[[nodiscard]] bool engineBuiltWithExceptions() noexcept`
  /// True if the engine translation unit was compiled with exceptions enabled.
- `[[nodiscard]] bool engineBuiltWithRtti() noexcept`
  /// True if the engine translation unit was compiled with RTTI enabled.
- `[[nodiscard]] long engineLibcxxVersion() noexcept`
  /// _LIBCPP_VERSION seen by the engine translation unit, or 0 if not built against libc++.

## `modules/core/include/axiom/core/error.hpp` — Engine-wide error type (CONVENTIONS §2). Never call .value() on a Result.
- `enum class ErrorCode : std::uint32_t`
  - `kUnknown = 0, kInvalidArgument, kOutOfMemory, kUnsupported, kIo, kGpu`
- `struct Error`
  /// @lifetime message must point to static storage (string literal).
  - `ErrorCode code = ErrorCode::kUnknown`
  - `std::string_view message`
- `template <class T> using Result = std::expected<T, Error>`
- `[[nodiscard]] constexpr std::unexpected<Error> fail(ErrorCode code, std::string_view message)`
  /// Builds the error branch of a Result.
