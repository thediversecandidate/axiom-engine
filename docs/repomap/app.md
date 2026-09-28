# Module `app` — public API
Depends on: ai, core, math, physics, renderer, renderer_vk

## `modules/app/include/axiom/app/module_info.hpp` — Stage 0 placeholder API for the app module. Allowed dependencies: axiom::core axiom::math axiom::physics
- `[[nodiscard]] std::string_view moduleName() noexcept`
  /// Name of this module, used by the Stage 0 smoke test.
