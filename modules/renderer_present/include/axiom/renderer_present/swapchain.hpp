#pragma once
// Swapchain for the window path (ROADMAP Stage 1): SRGB color output (CONVENTIONS §4), FIFO presentation,
// one frame in flight (every frame is submitted with CommandContext::submitAndWait before the next acquire).

#include "axiom/core/error.hpp"
#include "axiom/renderer_vk/vulkan_device.hpp"

#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::renderer {

struct SwapchainDesc {
  VkSurfaceKHR surface = VK_NULL_HANDLE; // the surface the device was created for (DeviceDesc::presentSurface)
  std::uint32_t width = 0;               // used when the surface does not fix the extent
  std::uint32_t height = 0;
  VkImageUsageFlags extraUsage = 0; // e.g. TRANSFER_SRC for readback in tests
};

/// One acquired image. The caller waits on `acquired` before color output and signals `renderFinished`.
struct SwapchainFrame {
  std::uint32_t index = 0;
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkSemaphore acquired = VK_NULL_HANDLE;
  VkSemaphore renderFinished = VK_NULL_HANDLE;
};

/// @owns the VkSwapchainKHR, one SRGB view and one render-finished semaphore per image, and the
///       image-acquired semaphore; move-only. Does not own the surface.
/// @lifetime must be destroyed before the device; the surface must outlive it.
/// @thread single-threaded.
/// @errors create()/recreate() fail with kUnsupported if the surface offers no SRGB format and the device has
///         no mutable swapchain format, kInvalidArgument for a zero extent, kGpu on any other Vulkan failure.
class Swapchain {
public:
  [[nodiscard]] static core::Result<Swapchain> create(const VulkanDevice &device, const SwapchainDesc &desc);

  Swapchain() = default;
  Swapchain(Swapchain &&other) noexcept;
  Swapchain &operator=(Swapchain &&other) noexcept;
  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;
  ~Swapchain();

  /// Rebuilds for a new extent (after a resize, or when acquire()/present() report out-of-date).
  [[nodiscard]] core::Result<bool> recreate(std::uint32_t width, std::uint32_t height);
  /// nullopt: out of date, call recreate(). Requires the previous frame's submission to have completed.
  [[nodiscard]] core::Result<std::optional<SwapchainFrame>> acquire();
  /// false: out of date or suboptimal, call recreate().
  [[nodiscard]] core::Result<bool> present(const SwapchainFrame &frame);

  /// Records: UNDEFINED → COLOR_ATTACHMENT_OPTIMAL, vkCmdBeginRendering clearing to `clearLinear`.
  void recordBegin(VkCommandBuffer cmd, const SwapchainFrame &frame, const float (&clearLinear)[4]) const noexcept;
  /// Records: vkCmdEndRendering; if `readback` is set, a copy of the image into it (tightly packed,
  /// extent().width * height * 4 bytes, requires TRANSFER_SRC usage); then → PRESENT_SRC_KHR.
  void recordEnd(VkCommandBuffer cmd, const SwapchainFrame &frame, VkBuffer readback = VK_NULL_HANDLE) const noexcept;

  /// The SRGB format of the image views: create pipelines with this format.
  [[nodiscard]] VkFormat viewFormat() const noexcept { return viewFormat_; }
  /// The format of the swapchain images (UNORM when rendered through mutable-format SRGB views).
  [[nodiscard]] VkFormat imageFormat() const noexcept { return imageFormat_; }
  [[nodiscard]] VkExtent2D extent() const noexcept { return extent_; }
  [[nodiscard]] std::uint32_t imageCount() const noexcept { return static_cast<std::uint32_t>(images_.size()); }

private:
  [[nodiscard]] core::Result<bool> build(std::uint32_t width, std::uint32_t height);
  void destroyImages() noexcept;
  void reset() noexcept;

  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  bool mutableFormat_ = false;
  VkImageUsageFlags extraUsage_ = 0;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat imageFormat_ = VK_FORMAT_UNDEFINED;
  VkFormat viewFormat_ = VK_FORMAT_UNDEFINED;
  VkExtent2D extent_{};
  std::vector<VkImage> images_;
  std::vector<VkImageView> views_;
  std::vector<VkSemaphore> renderFinished_;
  VkSemaphore acquired_ = VK_NULL_HANDLE;
};

} // namespace axiom::renderer
