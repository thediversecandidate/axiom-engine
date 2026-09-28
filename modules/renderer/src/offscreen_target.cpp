#include "axiom/renderer/offscreen_target.hpp"

#include <utility>

namespace axiom::renderer {

using core::ErrorCode;
using core::fail;

core::Result<OffscreenTarget> OffscreenTarget::create(const VulkanDevice &device, const GpuAllocator &allocator,
                                                      std::uint32_t width, std::uint32_t height) {
  OffscreenTarget t;
  t.device_ = device.handle();
  t.allocator_ = allocator.handle();
  t.width_ = width;
  t.height_ = height;

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = kFormat;
  imageInfo.extent = {width, height, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VmaAllocationCreateInfo gpuOnly{};
  gpuOnly.usage = VMA_MEMORY_USAGE_AUTO;
  if (vmaCreateImage(t.allocator_, &imageInfo, &gpuOnly, &t.image_, &t.imageAlloc_, nullptr) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vmaCreateImage failed");
  }

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = t.image_;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = kFormat;
  viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  if (vkCreateImageView(t.device_, &viewInfo, nullptr, &t.view_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkCreateImageView failed");
  }

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = static_cast<VkDeviceSize>(width) * height * sizeof(Rgba8);
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  VmaAllocationCreateInfo hostRead{};
  hostRead.usage = VMA_MEMORY_USAGE_AUTO;
  hostRead.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
  VmaAllocationInfo mappedInfo{};
  if (vmaCreateBuffer(t.allocator_, &bufferInfo, &hostRead, &t.readback_, &t.readbackAlloc_, &mappedInfo) !=
      VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "readback vmaCreateBuffer failed");
  }
  t.mapped_ = mappedInfo.pMappedData;
  return t;
}

void OffscreenTarget::recordBegin(VkCommandBuffer cmd, const float (&clearLinear)[4]) const noexcept {
  VkImageMemoryBarrier2 toAttachment{};
  toAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  toAttachment.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
  toAttachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  toAttachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  toAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  toAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  toAttachment.image = image_;
  toAttachment.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  VkDependencyInfo dep{};
  dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep.imageMemoryBarrierCount = 1;
  dep.pImageMemoryBarriers = &toAttachment;
  vkCmdPipelineBarrier2(cmd, &dep);

  VkRenderingAttachmentInfo color{};
  color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color.imageView = view_;
  color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.clearValue.color = {{clearLinear[0], clearLinear[1], clearLinear[2], clearLinear[3]}};
  VkRenderingInfo rendering{};
  rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  rendering.renderArea = {{0, 0}, {width_, height_}};
  rendering.layerCount = 1;
  rendering.colorAttachmentCount = 1;
  rendering.pColorAttachments = &color;
  vkCmdBeginRendering(cmd, &rendering);
}

void OffscreenTarget::recordEndAndReadback(VkCommandBuffer cmd) const noexcept {
  vkCmdEndRendering(cmd);

  VkImageMemoryBarrier2 toTransfer{};
  toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  toTransfer.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  toTransfer.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  toTransfer.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
  toTransfer.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
  toTransfer.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  toTransfer.image = image_;
  toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  VkDependencyInfo dep{};
  dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep.imageMemoryBarrierCount = 1;
  dep.pImageMemoryBarriers = &toTransfer;
  vkCmdPipelineBarrier2(cmd, &dep);

  VkBufferImageCopy region{};
  region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  region.imageExtent = {width_, height_, 1};
  vkCmdCopyImageToBuffer(cmd, image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback_, 1, &region);

  VkBufferMemoryBarrier2 toHost{};
  toHost.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  toHost.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
  toHost.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  toHost.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
  toHost.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
  toHost.buffer = readback_;
  toHost.size = VK_WHOLE_SIZE;
  VkDependencyInfo hostDep{};
  hostDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  hostDep.bufferMemoryBarrierCount = 1;
  hostDep.pBufferMemoryBarriers = &toHost;
  vkCmdPipelineBarrier2(cmd, &hostDep);
}

std::span<const Rgba8> OffscreenTarget::pixels() const noexcept {
  vmaInvalidateAllocation(allocator_, readbackAlloc_, 0, VK_WHOLE_SIZE);
  return {static_cast<const Rgba8 *>(mapped_), static_cast<std::size_t>(width_) * height_};
}

OffscreenTarget::OffscreenTarget(OffscreenTarget &&other) noexcept { *this = std::move(other); }

OffscreenTarget &OffscreenTarget::operator=(OffscreenTarget &&other) noexcept {
  if (this != &other) {
    reset();
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    allocator_ = std::exchange(other.allocator_, VK_NULL_HANDLE);
    image_ = std::exchange(other.image_, VK_NULL_HANDLE);
    imageAlloc_ = std::exchange(other.imageAlloc_, nullptr);
    view_ = std::exchange(other.view_, VK_NULL_HANDLE);
    readback_ = std::exchange(other.readback_, VK_NULL_HANDLE);
    readbackAlloc_ = std::exchange(other.readbackAlloc_, nullptr);
    mapped_ = std::exchange(other.mapped_, nullptr);
    width_ = other.width_;
    height_ = other.height_;
  }
  return *this;
}

OffscreenTarget::~OffscreenTarget() { reset(); }

void OffscreenTarget::reset() noexcept {
  if (view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(device_, view_, nullptr);
    view_ = VK_NULL_HANDLE;
  }
  if (image_ != VK_NULL_HANDLE) {
    vmaDestroyImage(allocator_, image_, imageAlloc_);
    image_ = VK_NULL_HANDLE;
  }
  if (readback_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(allocator_, readback_, readbackAlloc_);
    readback_ = VK_NULL_HANDLE;
  }
  mapped_ = nullptr;
}

} // namespace axiom::renderer
