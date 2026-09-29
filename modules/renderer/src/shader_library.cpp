#include "axiom/renderer/shader_library.hpp"

namespace axiom::renderer {
namespace {

// clang-format off
constexpr std::uint32_t kTriangleVertSpirv[] =
#include "triangle.vert.spv.inc"
;
constexpr std::uint32_t kTriangleFragSpirv[] =
#include "triangle.frag.spv.inc"
;
// clang-format on

} // namespace

std::span<const std::uint32_t> shaderSpirv(ShaderId id) noexcept {
  switch (id) {
  case ShaderId::kTriangleVert:
    return kTriangleVertSpirv;
  case ShaderId::kTriangleFrag:
    return kTriangleFragSpirv;
  }
  return {};
}

} // namespace axiom::renderer
