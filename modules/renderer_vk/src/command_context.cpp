#include "axiom/renderer_vk/command_context.hpp"

#include <cstdint>
#include <utility>

namespace axiom::renderer {

using core::ErrorCode;
using core::fail;

core::Result<CommandContext> CommandContext::create(const VulkanDevice &device) {
  CommandContext ctx;
  ctx.device_ = device.handle();
  ctx.queue_ = device.graphicsQueue();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = device.graphicsQueueFamily();
  if (vkCreateCommandPool(ctx.device_, &poolInfo, nullptr, &ctx.pool_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkCreateCommandPool failed");
  }

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = ctx.pool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(ctx.device_, &allocInfo, &ctx.cmd_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkAllocateCommandBuffers failed");
  }

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(ctx.device_, &fenceInfo, nullptr, &ctx.fence_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkCreateFence failed");
  }
  return ctx;
}

core::Result<VkCommandBuffer> CommandContext::begin() {
  if (vkResetCommandBuffer(cmd_, 0) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkResetCommandBuffer failed");
  }
  VkCommandBufferBeginInfo info{};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(cmd_, &info) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkBeginCommandBuffer failed");
  }
  return cmd_;
}

core::Result<bool> CommandContext::submitAndWait() {
  if (vkEndCommandBuffer(cmd_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkEndCommandBuffer failed");
  }
  VkCommandBufferSubmitInfo cmdInfo{};
  cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdInfo.commandBuffer = cmd_;
  VkSubmitInfo2 submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos = &cmdInfo;
  if (vkResetFences(device_, 1, &fence_) != VK_SUCCESS || vkQueueSubmit2(queue_, 1, &submit, fence_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkQueueSubmit2 failed");
  }
  if (vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkWaitForFences failed");
  }
  return true;
}

CommandContext::CommandContext(CommandContext &&other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)), queue_(std::exchange(other.queue_, VK_NULL_HANDLE)),
      pool_(std::exchange(other.pool_, VK_NULL_HANDLE)), cmd_(std::exchange(other.cmd_, VK_NULL_HANDLE)),
      fence_(std::exchange(other.fence_, VK_NULL_HANDLE)) {}

CommandContext &CommandContext::operator=(CommandContext &&other) noexcept {
  if (this != &other) {
    reset();
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    queue_ = std::exchange(other.queue_, VK_NULL_HANDLE);
    pool_ = std::exchange(other.pool_, VK_NULL_HANDLE);
    cmd_ = std::exchange(other.cmd_, VK_NULL_HANDLE);
    fence_ = std::exchange(other.fence_, VK_NULL_HANDLE);
  }
  return *this;
}

CommandContext::~CommandContext() { reset(); }

void CommandContext::reset() noexcept {
  if (device_ == VK_NULL_HANDLE) {
    return;
  }
  if (fence_ != VK_NULL_HANDLE) {
    vkDestroyFence(device_, fence_, nullptr);
  }
  if (pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, pool_, nullptr); // frees cmd_
  }
  device_ = VK_NULL_HANDLE;
  pool_ = VK_NULL_HANDLE;
  cmd_ = VK_NULL_HANDLE;
  fence_ = VK_NULL_HANDLE;
}

} // namespace axiom::renderer
