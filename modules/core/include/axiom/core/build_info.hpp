#pragma once
// Facts about how the engine library itself was compiled. Stage 0 tests use these to prove
// the engine is built without exceptions/RTTI and against libstdc++ (CONVENTIONS §1–2, ADR-0004).

namespace axiom::core {

/// True if the engine translation unit was compiled with exceptions enabled.
[[nodiscard]] bool engineBuiltWithExceptions() noexcept;

/// True if the engine translation unit was compiled with RTTI enabled.
[[nodiscard]] bool engineBuiltWithRtti() noexcept;

/// __GLIBCXX__ (libstdc++ release date) seen by the engine translation unit, or 0 if not libstdc++.
[[nodiscard]] long engineLibstdcxxVersion() noexcept;

} // namespace axiom::core
