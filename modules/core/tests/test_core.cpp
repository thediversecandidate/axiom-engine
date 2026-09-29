#include "axiom/core/build_info.hpp"
#include "axiom/core/error.hpp"

#include <catch2/catch_test_macros.hpp>
#include <version>

#if !defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
#error "Tests must be built against libstdc++, never libc++ (CONVENTIONS §1, ADR-0004)"
#endif

using axiom::core::ErrorCode;
using axiom::core::Result;

namespace {
Result<int> halve(int value) {
  if (value % 2 != 0) {
    return axiom::core::fail(ErrorCode::kInvalidArgument, "odd input");
  }
  return value / 2;
}
} // namespace

TEST_CASE("engine library is built without exceptions or RTTI", "[core][stage0]") {
  CHECK_FALSE(axiom::core::engineBuiltWithExceptions());
  CHECK_FALSE(axiom::core::engineBuiltWithRtti());
}

TEST_CASE("engine and tests are built against the same libstdc++", "[core][stage0]") {
  CHECK(axiom::core::engineLibstdcxxVersion() > 0);
  CHECK(axiom::core::engineLibstdcxxVersion() == __GLIBCXX__);
}

TEST_CASE("Result carries either a value or an Error", "[core]") {
  const Result<int> ok = halve(8);
  REQUIRE(ok.has_value());
  CHECK(*ok == 4);

  const Result<int> bad = halve(3);
  REQUIRE_FALSE(bad.has_value());
  CHECK(bad.error().code == ErrorCode::kInvalidArgument);
  CHECK(bad.error().message == "odd input");
}
