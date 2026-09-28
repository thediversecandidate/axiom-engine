#pragma once
// Stage 0 placeholder API for the app module. Allowed dependencies: axiom::core axiom::math axiom::physics
// axiom::renderer axiom::ai.

#include <string_view>

namespace axiom::app {

/// Name of this module, used by the Stage 0 smoke test.
[[nodiscard]] std::string_view moduleName() noexcept;

} // namespace axiom::app
