#include "axiom/renderer_vk/vulkan_device.hpp"

#include <cstring>
#include <utility>
#include <vector>

namespace axiom::renderer {
namespace {

using core::ErrorCode;
using core::fail;

struct Candidate {
  VkPhysicalDevice device = VK_NULL_HANDLE;
  std::uint32_t queueFamily = 0;
  VkDriverId driverId = static_cast<VkDriverId>(0);
  bool discrete = false;
};

// First graphics family; with a present surface, the first graphics family that can also present to it.
std::optional<std::uint32_t> graphicsFamily(VkPhysicalDevice device, VkSurfaceKHR surface) {
  std::uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  std::vector<VkQueueFamilyProperties> families(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
  for (std::uint32_t i = 0; i < count; ++i) {
    if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      continue;
    }
    VkBool32 present = VK_TRUE;
    if (surface != VK_NULL_HANDLE && vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present) != VK_SUCCESS) {
      present = VK_FALSE;
    }
    if (present) {
      return i;
    }
  }
  return std::nullopt;
}

bool deviceExtensionAvailable(VkPhysicalDevice device, const char *name) {
  std::uint32_t count = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> extensions(count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
  for (const auto &extension : extensions) {
    if (std::strcmp(extension.extensionName, name) == 0) {
      return true;
    }
  }
  return false;
}

std::optional<Candidate> evaluate(VkPhysicalDevice device, const DeviceDesc &desc) {
  VkPhysicalDeviceDriverProperties driver{};
  driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
  VkPhysicalDeviceProperties2 props{};
  props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  props.pNext = &driver;
  vkGetPhysicalDeviceProperties2(device, &props);
  if (props.properties.apiVersion < VK_API_VERSION_1_3) {
    return std::nullopt;
  }
  if (desc.requiredDriver && driver.driverID != *desc.requiredDriver) {
    return std::nullopt;
  }

  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &features13;
  vkGetPhysicalDeviceFeatures2(device, &features);
  if (!features13.dynamicRendering || !features13.synchronization2) {
    return std::nullopt;
  }

  if (desc.presentSurface != VK_NULL_HANDLE && !deviceExtensionAvailable(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
    return std::nullopt;
  }
  const auto family = graphicsFamily(device, desc.presentSurface);
  if (!family) {
    return std::nullopt;
  }
  return Candidate{device, *family, driver.driverID,
                   props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU};
}

} // namespace

core::Result<VulkanDevice> VulkanDevice::create(const VulkanInstance &instance, const DeviceDesc &desc) {
  std::uint32_t count = 0;
  vkEnumeratePhysicalDevices(instance.handle(), &count, nullptr);
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(instance.handle(), &count, devices.data());

  std::optional<Candidate> chosen;
  for (VkPhysicalDevice device : devices) {
    const auto candidate = evaluate(device, desc);
    if (candidate && (!chosen || (candidate->discrete && !chosen->discrete))) {
      chosen = candidate;
    }
  }
  if (!chosen) {
    return fail(ErrorCode::kUnsupported, "no Vulkan 1.3 device with dynamicRendering + synchronization2"
                                         " (and presentation to the surface, if requested)");
  }

  const float priority = 1.0f;
  VkDeviceQueueCreateInfo queueInfo{};
  queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueInfo.queueFamilyIndex = chosen->queueFamily;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &priority;

  // Explicit enables (CONVENTIONS §4): Vulkan 1.3 support alone does not turn these on.
  VkPhysicalDeviceVulkan13Features enable13{};
  enable13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  enable13.dynamicRendering = VK_TRUE;
  enable13.synchronization2 = VK_TRUE;
  VkPhysicalDeviceFeatures2 enable{};
  enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  enable.pNext = &enable13;

  VkDeviceCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.pNext = &enable;
  info.queueCreateInfoCount = 1;
  info.pQueueCreateInfos = &queueInfo;
  const char *swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  const bool present = desc.presentSurface != VK_NULL_HANDLE;
  if (present) {
    info.enabledExtensionCount = 1;
    info.ppEnabledExtensionNames = &swapchainExtension;
  }

  VulkanDevice result;
  if (vkCreateDevice(chosen->device, &info, nullptr, &result.device_) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkCreateDevice failed");
  }
  result.physical_ = chosen->device;
  result.queueFamily_ = chosen->queueFamily;
  result.driverId_ = chosen->driverId;
  result.presentEnabled_ = present;
  vkGetDeviceQueue(result.device_, result.queueFamily_, 0, &result.queue_);
  return result;
}

VulkanDevice::VulkanDevice(VulkanDevice &&other) noexcept
    : physical_(std::exchange(other.physical_, VK_NULL_HANDLE)), device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      queue_(std::exchange(other.queue_, VK_NULL_HANDLE)), queueFamily_(other.queueFamily_), driverId_(other.driverId_),
      presentEnabled_(std::exchange(other.presentEnabled_, false)) {}

VulkanDevice &VulkanDevice::operator=(VulkanDevice &&other) noexcept {
  if (this != &other) {
    reset();
    physical_ = std::exchange(other.physical_, VK_NULL_HANDLE);
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    queue_ = std::exchange(other.queue_, VK_NULL_HANDLE);
    queueFamily_ = other.queueFamily_;
    driverId_ = other.driverId_;
    presentEnabled_ = std::exchange(other.presentEnabled_, false);
  }
  return *this;
}

VulkanDevice::~VulkanDevice() { reset(); }

void VulkanDevice::reset() noexcept {
  if (device_ != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device_);
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
  physical_ = VK_NULL_HANDLE;
  queue_ = VK_NULL_HANDLE;
  presentEnabled_ = false;
}

} // namespace axiom::renderer
