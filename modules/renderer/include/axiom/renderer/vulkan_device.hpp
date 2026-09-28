#pragma once
// Physical-device selection and logical-device creation with explicit Vulkan 1.3 feature enables.

#include "axiom/core/error.hpp"
#include "axiom/renderer/vulkan_instance.hpp"

#include <cstdint>
#include <optional>
#include <vulkan/vulkan.h>

namespace axiom::renderer {

struct DeviceDesc {
  /// If set, only a device whose VkPhysicalDeviceDriverProperties::driverID matches is accepted
  /// (CI uses VK_DRIVER_ID_MESA_LLVMPIPE to assert lavapipe identity).
  std::optional<VkDriverId> requiredDriver;
};

/// @owns the VkDevice; move-only.
/// @lifetime must be destroyed before the VulkanInstance it was created from.
/// @errors create() fails with kUnsupported if no device offers Vulkan 1.3, a graphics queue,
///         dynamicRendering and synchronization2 (and the required driver, if any).
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

private:
  void reset() noexcept;

  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  std::uint32_t queueFamily_ = 0;
  VkDriverId driverId_ = static_cast<VkDriverId>(0);
};

} // namespace axiom::renderer
