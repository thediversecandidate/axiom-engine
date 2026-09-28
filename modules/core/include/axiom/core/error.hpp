#pragma once
// Engine-wide error type (CONVENTIONS §2). Never call .value() on a Result.

#include <cstdint>
#include <expected>
#include <string_view>

namespace axiom::core {

enum class ErrorCode : std::uint32_t {
  kUnknown = 0,
  kInvalidArgument,
  kOutOfMemory,
  kUnsupported,
  kIo,
  kGpu,
};

/// @lifetime message must point to static storage (string literal).
struct Error {
  ErrorCode code = ErrorCode::kUnknown;
  std::string_view message;
};

template <class T> using Result = std::expected<T, Error>;

/// Builds the error branch of a Result.
[[nodiscard]] constexpr std::unexpected<Error> fail(ErrorCode code, std::string_view message) {
  return std::unexpected<Error>(Error{code, message});
}

} // namespace axiom::core
