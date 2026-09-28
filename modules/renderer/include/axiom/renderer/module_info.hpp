#pragma once
// Stage 0 placeholder API for the renderer module. Allowed dependencies: axiom::core axiom::math.

#include <string_view>

namespace axiom::renderer {

/// Name of this module, used by the Stage 0 smoke test.
[[nodiscard]] std::string_view moduleName() noexcept;

} // namespace axiom::renderer
