#include "axiom/renderer/vulkan_instance.hpp"

#include <array>
#include <cstring>
#include <utility>
#include <vector>

namespace axiom::renderer {
namespace {

constexpr const char *kValidationLayer = "VK_LAYER_KHRONOS_validation";
using core::ErrorCode;
using core::fail;

VKAPI_ATTR VkBool32 VKAPI_CALL onDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                              VkDebugUtilsMessageTypeFlagsEXT /*types*/,
                                              const VkDebugUtilsMessengerCallbackDataEXT *data, void *user) {
  auto *sink = static_cast<ValidationSink *>(user);
  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    sink->errors.fetch_add(1, std::memory_order_relaxed);
    bool expected = false;
    if (sink->firstErrorSet.compare_exchange_strong(expected, true) && data && data->pMessage) {
      std::strncpy(sink->firstError, data->pMessage, sizeof(sink->firstError) - 1);
    }
  } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    sink->warnings.fetch_add(1, std::memory_order_relaxed);
  } else {
    sink->infos.fetch_add(1, std::memory_order_relaxed);
  }
  return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT messengerInfo(ValidationSink *sink) {
  VkDebugUtilsMessengerCreateInfoEXT info{};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
  info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  info.pfnUserCallback = onDebugMessage;
  info.pUserData = sink;
  return info;
}

bool layerAvailable(const char *name) {
  std::uint32_t count = 0;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  std::vector<VkLayerProperties> layers(count);
  vkEnumerateInstanceLayerProperties(&count, layers.data());
  for (const auto &layer : layers) {
    if (std::strcmp(layer.layerName, name) == 0) {
      return true;
    }
  }
  return false;
}

} // namespace

core::Result<VulkanInstance> VulkanInstance::create(const InstanceDesc &desc) {
  std::uint32_t loaderVersion = 0;
  if (vkEnumerateInstanceVersion(&loaderVersion) != VK_SUCCESS || loaderVersion < VK_API_VERSION_1_3) {
    return fail(ErrorCode::kUnsupported, "Vulkan loader does not support Vulkan 1.3");
  }
  if (desc.enableValidation && desc.sink == nullptr) {
    return fail(ErrorCode::kInvalidArgument, "validation requires a ValidationSink");
  }
  if (desc.enableValidation && !layerAvailable(kValidationLayer)) {
    return fail(ErrorCode::kUnsupported, "VK_LAYER_KHRONOS_validation is not installed");
  }

  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = desc.appName;
  app.pEngineName = "axiom";
  app.apiVersion = VK_API_VERSION_1_3;

  const std::array<const char *, 1> layers{kValidationLayer};
  const std::array<const char *, 1> extensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  // Chained so messages emitted during vkCreateInstance / vkDestroyInstance are also counted.
  VkDebugUtilsMessengerCreateInfoEXT chained = messengerInfo(desc.sink);

  VkInstanceCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.pApplicationInfo = &app;
  if (desc.enableValidation) {
    info.pNext = &chained;
    info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
    info.ppEnabledLayerNames = layers.data();
    info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
  }

  VulkanInstance result;
  if (vkCreateInstance(&info, nullptr, &result.instance_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkCreateInstance failed");
  }
  if (desc.enableValidation) {
    auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(result.instance_, "vkCreateDebugUtilsMessengerEXT"));
    const VkDebugUtilsMessengerCreateInfoEXT persistent = messengerInfo(desc.sink);
    if (create == nullptr || create(result.instance_, &persistent, nullptr, &result.messenger_) != VK_SUCCESS) {
      return fail(ErrorCode::kGpu, "vkCreateDebugUtilsMessengerEXT failed");
    }
  }
  return result;
}

VulkanInstance::VulkanInstance(VulkanInstance &&other) noexcept
    : instance_(std::exchange(other.instance_, VK_NULL_HANDLE)),
      messenger_(std::exchange(other.messenger_, VK_NULL_HANDLE)) {}

VulkanInstance &VulkanInstance::operator=(VulkanInstance &&other) noexcept {
  if (this != &other) {
    reset();
    instance_ = std::exchange(other.instance_, VK_NULL_HANDLE);
    messenger_ = std::exchange(other.messenger_, VK_NULL_HANDLE);
  }
  return *this;
}

VulkanInstance::~VulkanInstance() { reset(); }

void VulkanInstance::reset() noexcept {
  if (messenger_ != VK_NULL_HANDLE) {
    auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroy != nullptr) {
      destroy(instance_, messenger_, nullptr);
    }
    messenger_ = VK_NULL_HANDLE;
  }
  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

void VulkanInstance::submitTestMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                       const char *text) const noexcept {
  auto submit = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(
      vkGetInstanceProcAddr(instance_, "vkSubmitDebugUtilsMessageEXT"));
  if (submit == nullptr) {
    return;
  }
  VkDebugUtilsMessengerCallbackDataEXT data{};
  data.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
  data.pMessage = text;
  submit(instance_, severity, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, &data);
}

} // namespace axiom::renderer
