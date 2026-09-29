#pragma once
// Stage 1 triangle pipeline: dynamic rendering, vertex = {vec3 position, vec3 linear color},
// MVP as a 64-byte vertex push constant, dynamic viewport/scissor/front face, back-face culling.

#include "axiom/core/error.hpp"
#include "axiom/renderer_vk/vulkan_device.hpp"

#include <vulkan/vulkan.h>

namespace axiom::renderer {

struct TriangleVertex {
  float position[3];
  float color[3]; // linear RGB
};

/// @owns the VkPipeline and its VkPipelineLayout; move-only.
/// @lifetime must be destroyed before the device.
/// @errors create() fails with kGpu if shader-module, layout or pipeline creation fails.
class TrianglePipeline {
public:
  [[nodiscard]] static core::Result<TrianglePipeline> create(const VulkanDevice &device, VkFormat colorFormat);

  TrianglePipeline() = default;
  TrianglePipeline(TrianglePipeline &&other) noexcept;
  TrianglePipeline &operator=(TrianglePipeline &&other) noexcept;
  TrianglePipeline(const TrianglePipeline &) = delete;
  TrianglePipeline &operator=(const TrianglePipeline &) = delete;
  ~TrianglePipeline();

  /// Records: bind pipeline, viewport/scissor over (width, height), front face, MVP push constant.
  /// frontFace must be COUNTER_CLOCKWISE unless the world transform's determinant is negative (CONVENTIONS §4).
  void recordBind(VkCommandBuffer cmd, std::uint32_t width, std::uint32_t height, VkFrontFace frontFace,
                  const float (&mvpColumnMajor)[16]) const noexcept;

private:
  void reset() noexcept;

  VkDevice device_ = VK_NULL_HANDLE;
  VkPipelineLayout layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

} // namespace axiom::renderer
