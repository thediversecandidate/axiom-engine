#pragma once
// Stage 0 placeholder API for the ai module. Allowed dependencies: axiom::core axiom::math.

#include <string_view>

namespace axiom::ai {

/// Name of this module, used by the Stage 0 smoke test.
[[nodiscard]] std::string_view moduleName() noexcept;

} // namespace axiom::ai
