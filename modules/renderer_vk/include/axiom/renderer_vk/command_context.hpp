#pragma once
// One command pool + command buffer + fence for synchronous submission (tests, offscreen renders).

#include "axiom/core/error.hpp"
#include "axiom/renderer_vk/vulkan_device.hpp"

#include <vulkan/vulkan.h>

namespace axiom::renderer {

/// @owns a command pool, one primary command buffer and a fence; move-only.
/// @lifetime must be destroyed before the device.
/// @thread single-threaded: begin() and submitAndWait() on one thread.
/// @errors begin()/submitAndWait() return kGpu on any Vulkan failure.
class CommandContext {
public:
  [[nodiscard]] static core::Result<CommandContext> create(const VulkanDevice &device);

  CommandContext() = default;
  CommandContext(CommandContext &&other) noexcept;
  CommandContext &operator=(CommandContext &&other) noexcept;
  CommandContext(const CommandContext &) = delete;
  CommandContext &operator=(const CommandContext &) = delete;
  ~CommandContext();

  /// Resets and begins the command buffer for one-time submission.
  [[nodiscard]] core::Result<VkCommandBuffer> begin();
  /// Ends, submits to the graphics queue and blocks until the GPU finishes.
  [[nodiscard]] core::Result<bool> submitAndWait();

private:
  void reset() noexcept;

  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  VkCommandPool pool_ = VK_NULL_HANDLE;
  VkCommandBuffer cmd_ = VK_NULL_HANDLE;
  VkFence fence_ = VK_NULL_HANDLE;
};

} // namespace axiom::renderer
