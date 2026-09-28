#include "axiom/renderer/triangle_pipeline.hpp"

#include "axiom/renderer/shader_library.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace axiom::renderer {
namespace {

VkShaderModule makeModule(VkDevice device, ShaderId id) {
  const auto words = shaderSpirv(id);
  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = words.size_bytes();
  info.pCode = words.data();
  VkShaderModule module = VK_NULL_HANDLE;
  vkCreateShaderModule(device, &info, nullptr, &module);
  return module;
}

} // namespace

core::Result<TrianglePipeline> TrianglePipeline::create(const VulkanDevice &device, VkFormat colorFormat) {
  TrianglePipeline p;
  p.device_ = device.handle();

  VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16};
  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &push;
  if (vkCreatePipelineLayout(p.device_, &layoutInfo, nullptr, &p.layout_) != VK_SUCCESS) {
    return core::fail(core::ErrorCode::kGpu, "vkCreatePipelineLayout failed");
  }

  const VkShaderModule vert = makeModule(p.device_, ShaderId::kTriangleVert);
  const VkShaderModule frag = makeModule(p.device_, ShaderId::kTriangleFrag);
  std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
  stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
               nullptr,
               0,
               VK_SHADER_STAGE_VERTEX_BIT,
               vert,
               "main",
               nullptr};
  stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
               nullptr,
               0,
               VK_SHADER_STAGE_FRAGMENT_BIT,
               frag,
               "main",
               nullptr};

  const VkVertexInputBindingDescription binding{0, sizeof(TriangleVertex), VK_VERTEX_INPUT_RATE_VERTEX};
  const std::array<VkVertexInputAttributeDescription, 2> attributes{{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TriangleVertex, position)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TriangleVertex, color)},
  }};
  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount = 1;
  vertexInput.pVertexBindingDescriptions = &binding;
  vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo assembly{};
  assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkPipelineViewportStateCreateInfo viewport{};
  viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;
  VkPipelineRasterizationStateCreateInfo raster{};
  raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster.polygonMode = VK_POLYGON_MODE_FILL;
  raster.cullMode = VK_CULL_MODE_BACK_BIT;
  raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // ignored: front face is dynamic state
  raster.lineWidth = 1.0f;
  VkPipelineMultisampleStateCreateInfo multisample{};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  VkPipelineColorBlendAttachmentState blendAttachment{};
  blendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo blend{};
  blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blend.attachmentCount = 1;
  blend.pAttachments = &blendAttachment;
  const std::array<VkDynamicState, 3> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                                                    VK_DYNAMIC_STATE_FRONT_FACE};
  VkPipelineDynamicStateCreateInfo dynamic{};
  dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
  dynamic.pDynamicStates = dynamicStates.data();
  VkPipelineRenderingCreateInfo rendering{};
  rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachmentFormats = &colorFormat;

  VkGraphicsPipelineCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.pNext = &rendering;
  info.stageCount = static_cast<std::uint32_t>(stages.size());
  info.pStages = stages.data();
  info.pVertexInputState = &vertexInput;
  info.pInputAssemblyState = &assembly;
  info.pViewportState = &viewport;
  info.pRasterizationState = &raster;
  info.pMultisampleState = &multisample;
  info.pColorBlendState = &blend;
  info.pDynamicState = &dynamic;
  info.layout = p.layout_;
  const VkResult result = vkCreateGraphicsPipelines(p.device_, VK_NULL_HANDLE, 1, &info, nullptr, &p.pipeline_);
  vkDestroyShaderModule(p.device_, vert, nullptr);
  vkDestroyShaderModule(p.device_, frag, nullptr);
  if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE || result != VK_SUCCESS) {
    return core::fail(core::ErrorCode::kGpu, "triangle pipeline creation failed");
  }
  return p;
}

void TrianglePipeline::recordBind(VkCommandBuffer cmd, std::uint32_t width, std::uint32_t height, VkFrontFace frontFace,
                                  const float (&mvpColumnMajor)[16]) const noexcept {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  // Positive height and depth range [0, 1]: the only Y inversion is in the projection (CONVENTIONS §4).
  const VkViewport viewport{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
  const VkRect2D scissor{{0, 0}, {width, height}};
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);
  vkCmdSetFrontFace(cmd, frontFace);
  vkCmdPushConstants(cmd, layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvpColumnMajor), mvpColumnMajor);
}

TrianglePipeline::TrianglePipeline(TrianglePipeline &&other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)), layout_(std::exchange(other.layout_, VK_NULL_HANDLE)),
      pipeline_(std::exchange(other.pipeline_, VK_NULL_HANDLE)) {}

TrianglePipeline &TrianglePipeline::operator=(TrianglePipeline &&other) noexcept {
  if (this != &other) {
    reset();
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    layout_ = std::exchange(other.layout_, VK_NULL_HANDLE);
    pipeline_ = std::exchange(other.pipeline_, VK_NULL_HANDLE);
  }
  return *this;
}

TrianglePipeline::~TrianglePipeline() { reset(); }

void TrianglePipeline::reset() noexcept {
  if (device_ == VK_NULL_HANDLE) {
    return;
  }
  if (pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, pipeline_, nullptr);
  }
  if (layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, layout_, nullptr);
  }
  device_ = VK_NULL_HANDLE;
  pipeline_ = VK_NULL_HANDLE;
  layout_ = VK_NULL_HANDLE;
}

} // namespace axiom::renderer
