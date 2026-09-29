#pragma once
// SPIR-V compiled from modules/renderer/shaders at build time (cmake/AxiomShaders.cmake).

#include <cstdint>
#include <span>

namespace axiom::renderer {

enum class ShaderId : std::uint8_t {
  kTriangleVert,
  kTriangleFrag,
};

/// SPIR-V words of a built-in shader.
/// @lifetime static storage; the span is valid for the whole program.
[[nodiscard]] std::span<const std::uint32_t> shaderSpirv(ShaderId id) noexcept;

} // namespace axiom::renderer
