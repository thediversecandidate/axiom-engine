#pragma once
// Physical-device selection and logical-device creation with explicit Vulkan 1.3 feature enables.

#include "axiom/core/error.hpp"
#include "axiom/renderer_vk/vulkan_instance.hpp"

#include <cstdint>
#include <optional>
#include <vulkan/vulkan.h>

namespace axiom::renderer {

struct DeviceDesc {
  /// If set, only a device whose VkPhysicalDeviceDriverProperties::driverID matches is accepted
  /// (CI uses VK_DRIVER_ID_MESA_LLVMPIPE to assert lavapipe identity).
  std::optional<VkDriverId> requiredDriver;
  /// If set, the device must offer VK_KHR_swapchain and a graphics queue family that can present to this
  /// surface; VK_KHR_swapchain is then enabled. The surface must outlive device creation.
  VkSurfaceKHR presentSurface = VK_NULL_HANDLE;
};

/// @owns the VkDevice; move-only.
/// @lifetime must be destroyed before the VulkanInstance it was created from.
/// @errors create() fails with kUnsupported if no device offers Vulkan 1.3, a graphics queue,
///         dynamicRendering and synchronization2 (and the required driver and presentation, if requested).
class VulkanDevice {
public:
  [[nodiscard]] static core::Result<VulkanDevice> create(const VulkanInstance &instance, const DeviceDesc &desc);

  VulkanDevice() = default;
  VulkanDevice(VulkanDevice &&other) noexcept;
  VulkanDevice &operator=(VulkanDevice &&other) noexcept;
  VulkanDevice(const VulkanDevice &) = delete;
  VulkanDevice &operator=(const VulkanDevice &) = delete;
  ~VulkanDevice();

  [[nodiscard]] VkDevice handle() const noexcept { return device_; }
  [[nodiscard]] VkPhysicalDevice physical() const noexcept { return physical_; }
  [[nodiscard]] VkQueue graphicsQueue() const noexcept { return queue_; }
  [[nodiscard]] std::uint32_t graphicsQueueFamily() const noexcept { return queueFamily_; }
  [[nodiscard]] VkDriverId driverId() const noexcept { return driverId_; }
  /// True if VK_KHR_swapchain is enabled and the graphics queue can present to DeviceDesc::presentSurface.
  [[nodiscard]] bool presentEnabled() const noexcept { return presentEnabled_; }
  /// True if VK_KHR_swapchain_mutable_format is enabled (requested with presentation when available), which
  /// lets a swapchain of UNORM images be rendered through SRGB views when the surface offers no SRGB format.
  [[nodiscard]] bool mutableSwapchainFormat() const noexcept { return mutableSwapchainFormat_; }

private:
  void reset() noexcept;

  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  std::uint32_t queueFamily_ = 0;
  VkDriverId driverId_ = static_cast<VkDriverId>(0);
  bool presentEnabled_ = false;
  bool mutableSwapchainFormat_ = false;
};

} // namespace axiom::renderer
