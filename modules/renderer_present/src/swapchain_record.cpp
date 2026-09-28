// Command recording for Swapchain (split from swapchain.cpp for the 300-line file limit).
#include "axiom/renderer_present/swapchain.hpp"

#include <algorithm>

namespace axiom::renderer {
namespace {

void imageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout from, VkImageLayout to,
                  VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage,
                  VkAccessFlags2 dstAccess) {
  VkImageMemoryBarrier2 b{};
  b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  b.srcStageMask = srcStage;
  b.srcAccessMask = srcAccess;
  b.dstStageMask = dstStage;
  b.dstAccessMask = dstAccess;
  b.oldLayout = from;
  b.newLayout = to;
  b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  b.image = image;
  b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  VkDependencyInfo dep{};
  dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep.imageMemoryBarrierCount = 1;
  dep.pImageMemoryBarriers = &b;
  vkCmdPipelineBarrier2(cmd, &dep);
}

} // namespace

void Swapchain::recordBegin(VkCommandBuffer cmd, const SwapchainFrame &frame,
                            const float (&clearLinear)[4]) const noexcept {
  // Source stage matches the semaphore wait stage (COLOR_ATTACHMENT_OUTPUT), so the layout transition
  // happens after the image is acquired.
  imageBarrier(cmd, frame.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
               VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
               VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  VkRenderingAttachmentInfo color{};
  color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color.imageView = frame.view;
  color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  std::copy(std::begin(clearLinear), std::end(clearLinear), color.clearValue.color.float32);
  VkRenderingInfo rendering{};
  rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  rendering.renderArea = {{0, 0}, extent_};
  rendering.layerCount = 1;
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachments = &color;
  vkCmdBeginRendering(cmd, &rendering);
}

void Swapchain::recordEnd(VkCommandBuffer cmd, const SwapchainFrame &frame, VkBuffer readback) const noexcept {
  vkCmdEndRendering(cmd);
  VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkAccessFlags2 access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  if (readback != VK_NULL_HANDLE) {
    imageBarrier(cmd, frame.image, layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage, access,
                 VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {extent_.width, extent_.height, 1};
    vkCmdCopyImageToBuffer(cmd, frame.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback, 1, &region);
    VkMemoryBarrier2 toHost{};
    toHost.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    toHost.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    toHost.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    toHost.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
    toHost.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.memoryBarrierCount = 1;
    dep.pMemoryBarriers = &toHost;
    vkCmdPipelineBarrier2(cmd, &dep);
    layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    stage = VK_PIPELINE_STAGE_2_COPY_BIT;
    access = 0;
  }
  // Presentation is ordered by the render-finished semaphore; no destination stage is needed.
  imageBarrier(cmd, frame.image, layout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, stage, access, VK_PIPELINE_STAGE_2_NONE, 0);
}

} // namespace axiom::renderer
