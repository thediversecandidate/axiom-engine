#pragma once
// Vulkan 1.3 instance with the Khronos validation layer and a debug-utils messenger.

#include "axiom/core/error.hpp"
#include "axiom/renderer_vk/validation.hpp"

#include <span>
#include <vulkan/vulkan.h>

namespace axiom::renderer {

struct InstanceDesc {
  const char *appName = "axiom";
  /// Enables VK_LAYER_KHRONOS_validation and VK_EXT_debug_utils; creation fails if unavailable.
  bool enableValidation = true;
  /// Receives validation counts. Required when enableValidation is true.
  ValidationSink *sink = nullptr;
  /// Additional instance extensions that must be enabled (e.g. from SDL_Vulkan_GetInstanceExtensions, or
  /// VK_KHR_surface + VK_EXT_headless_surface in tests). Creation fails with kUnsupported if one is missing.
  std::span<const char *const> extraExtensions;
};

/// @owns the VkInstance and its debug messenger; move-only.
/// @lifetime destroy every device created from it first; desc.sink must outlive it.
/// @errors create() fails with kUnsupported if Vulkan 1.3, the layer or the extension is missing.
class VulkanInstance {
public:
  [[nodiscard]] static core::Result<VulkanInstance> create(const InstanceDesc &desc);

  VulkanInstance() = default;
  VulkanInstance(VulkanInstance &&other) noexcept;
  VulkanInstance &operator=(VulkanInstance &&other) noexcept;
  VulkanInstance(const VulkanInstance &) = delete;
  VulkanInstance &operator=(const VulkanInstance &) = delete;
  ~VulkanInstance();

  [[nodiscard]] VkInstance handle() const noexcept { return instance_; }
  [[nodiscard]] bool validationEnabled() const noexcept { return messenger_ != VK_NULL_HANDLE; }

  /// Sends a message through the debug-utils path (used by tests to prove the sink is wired).
  void submitTestMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, const char *text) const noexcept;

private:
  void reset() noexcept;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
};

} // namespace axiom::renderer
