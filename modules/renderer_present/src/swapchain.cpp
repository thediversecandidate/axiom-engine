#include "axiom/renderer_present/swapchain.hpp"

#include <algorithm>
#include <utility>

namespace axiom::renderer {
namespace {

using core::ErrorCode;
using core::fail;

struct FormatChoice {
  VkFormat image = VK_FORMAT_UNDEFINED;
  VkFormat view = VK_FORMAT_UNDEFINED;
  VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
};

VkFormat srgbOf(VkFormat f) {
  switch (f) {
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8A8_SRGB:
    return VK_FORMAT_B8G8R8A8_SRGB;
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SRGB:
    return VK_FORMAT_R8G8B8A8_SRGB;
  default:
    return VK_FORMAT_UNDEFINED;
  }
}

// Prefers an SRGB surface format; otherwise an UNORM one viewed as SRGB (needs the mutable format extension).
std::optional<FormatChoice> chooseFormat(VkPhysicalDevice physical, VkSurfaceKHR surface, bool mutableFormat) {
  std::uint32_t count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, formats.data());
  for (const bool viaMutable : {false, true}) {
    for (const auto &f : formats) {
      const VkFormat srgb = srgbOf(f.format);
      if (f.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR || srgb == VK_FORMAT_UNDEFINED) {
        continue;
      }
      if (!viaMutable && f.format == srgb) {
        return FormatChoice{f.format, srgb, f.colorSpace};
      }
      if (viaMutable && mutableFormat) {
        return FormatChoice{f.format, srgb, f.colorSpace};
      }
    }
  }
  return std::nullopt;
}

VkSemaphore makeSemaphore(VkDevice device) {
  VkSemaphoreCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkSemaphore s = VK_NULL_HANDLE;
  return vkCreateSemaphore(device, &info, nullptr, &s) == VK_SUCCESS ? s : VK_NULL_HANDLE;
}

} // namespace

core::Result<Swapchain> Swapchain::create(const VulkanDevice &device, const SwapchainDesc &desc) {
  if (!device.presentEnabled() || desc.surface == VK_NULL_HANDLE) {
    return fail(ErrorCode::kInvalidArgument, "the device was not created for presentation to a surface");
  }
  Swapchain s;
  s.physical_ = device.physical();
  s.device_ = device.handle();
  s.queue_ = device.graphicsQueue();
  s.surface_ = desc.surface;
  s.mutableFormat_ = device.mutableSwapchainFormat();
  s.extraUsage_ = desc.extraUsage;
  s.acquired_ = makeSemaphore(s.device_);
  if (s.acquired_ == VK_NULL_HANDLE) {
    return fail(ErrorCode::kGpu, "vkCreateSemaphore failed");
  }
  if (auto built = s.build(desc.width, desc.height); !built) {
    return std::unexpected(built.error());
  }
  return s;
}

core::Result<bool> Swapchain::recreate(std::uint32_t width, std::uint32_t height) {
  vkDeviceWaitIdle(device_);
  return build(width, height);
}

core::Result<bool> Swapchain::build(std::uint32_t width, std::uint32_t height) {
  VkSurfaceCapabilitiesKHR caps{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_, surface_, &caps) != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
  }
  const auto format = chooseFormat(physical_, surface_, mutableFormat_);
  if (!format) {
    return fail(ErrorCode::kUnsupported, "no SRGB surface format and no mutable swapchain format");
  }
  const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | extraUsage_;
  if ((caps.supportedUsageFlags & usage) != usage) {
    return fail(ErrorCode::kUnsupported, "the surface does not support the requested image usage");
  }
  VkExtent2D extent = caps.currentExtent;
  if (extent.width == UINT32_MAX) {
    extent.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
  }
  if (extent.width == 0 || extent.height == 0) {
    return fail(ErrorCode::kInvalidArgument, "swapchain extent is zero (minimized window)");
  }
  std::uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0) {
    imageCount = std::min(imageCount, caps.maxImageCount);
  }

  const VkFormat viewFormats[2] = {format->image, format->view};
  VkImageFormatListCreateInfo formatList{};
  formatList.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
  formatList.viewFormatCount = 2;
  formatList.pViewFormats = viewFormats;
  VkSwapchainCreateInfoKHR info{};
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  if (format->image != format->view) {
    info.pNext = &formatList;
    info.flags = VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
  }
  info.surface = surface_;
  info.minImageCount = imageCount;
  info.imageFormat = format->image;
  info.imageColorSpace = format->colorSpace;
  info.imageExtent = extent;
  info.imageArrayLayers = 1;
  info.imageUsage = usage;
  info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.preTransform = caps.currentTransform;
  info.compositeAlpha =
      (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
          ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
          : static_cast<VkCompositeAlphaFlagBitsKHR>(caps.supportedCompositeAlpha & -caps.supportedCompositeAlpha);
  info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // always supported
  info.clipped = VK_TRUE;
  info.oldSwapchain = swapchain_;

  VkSwapchainKHR created = VK_NULL_HANDLE;
  const VkResult result = vkCreateSwapchainKHR(device_, &info, nullptr, &created);
  destroyImages();
  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }
  if (result != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkCreateSwapchainKHR failed");
  }
  swapchain_ = created;
  imageFormat_ = format->image;
  viewFormat_ = format->view;
  extent_ = extent;

  std::uint32_t count = 0;
  vkGetSwapchainImagesKHR(device_, swapchain_, &count, nullptr);
  images_.resize(count);
  vkGetSwapchainImagesKHR(device_, swapchain_, &count, images_.data());
  for (VkImage image : images_) {
    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = viewFormat_;
    view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkImageView v = VK_NULL_HANDLE;
    const VkSemaphore done = makeSemaphore(device_);
    if (vkCreateImageView(device_, &view, nullptr, &v) != VK_SUCCESS || done == VK_NULL_HANDLE) {
      if (done != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, done, nullptr);
      }
      return fail(ErrorCode::kGpu, "swapchain view or semaphore creation failed");
    }
    views_.push_back(v);
    renderFinished_.push_back(done);
  }
  return true;
}

core::Result<std::optional<SwapchainFrame>> Swapchain::acquire() {
  std::uint32_t index = 0;
  const VkResult r = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, acquired_, VK_NULL_HANDLE, &index);
  if (r == VK_ERROR_OUT_OF_DATE_KHR) {
    return std::optional<SwapchainFrame>{};
  }
  if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
    return fail(ErrorCode::kGpu, "vkAcquireNextImageKHR failed");
  }
  return std::optional{SwapchainFrame{index, images_[index], views_[index], acquired_, renderFinished_[index]}};
}

core::Result<bool> Swapchain::present(const SwapchainFrame &frame) {
  VkPresentInfoKHR info{};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &frame.renderFinished;
  info.swapchainCount = 1;
  info.pSwapchains = &swapchain_;
  info.pImageIndices = &frame.index;
  const VkResult r = vkQueuePresentKHR(queue_, &info);
  if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
    return false;
  }
  if (r != VK_SUCCESS) {
    return fail(ErrorCode::kGpu, "vkQueuePresentKHR failed");
  }
  return true;
}

Swapchain::Swapchain(Swapchain &&other) noexcept { *this = std::move(other); }

Swapchain &Swapchain::operator=(Swapchain &&other) noexcept {
  if (this != &other) {
    reset();
    physical_ = std::exchange(other.physical_, VK_NULL_HANDLE);
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    queue_ = std::exchange(other.queue_, VK_NULL_HANDLE);
    surface_ = std::exchange(other.surface_, VK_NULL_HANDLE);
    mutableFormat_ = other.mutableFormat_;
    extraUsage_ = other.extraUsage_;
    swapchain_ = std::exchange(other.swapchain_, VK_NULL_HANDLE);
    imageFormat_ = other.imageFormat_;
    viewFormat_ = other.viewFormat_;
    extent_ = other.extent_;
    images_ = std::move(other.images_);
    views_ = std::move(other.views_);
    renderFinished_ = std::move(other.renderFinished_);
    acquired_ = std::exchange(other.acquired_, VK_NULL_HANDLE);
    other.images_.clear();
    other.views_.clear();
    other.renderFinished_.clear();
  }
  return *this;
}

Swapchain::~Swapchain() { reset(); }

void Swapchain::destroyImages() noexcept {
  for (VkImageView v : views_) {
    vkDestroyImageView(device_, v, nullptr);
  }
  for (VkSemaphore s : renderFinished_) {
    vkDestroySemaphore(device_, s, nullptr);
  }
  views_.clear();
  renderFinished_.clear();
  images_.clear();
}

void Swapchain::reset() noexcept {
  if (device_ == VK_NULL_HANDLE) {
    return;
  }
  vkDeviceWaitIdle(device_);
  destroyImages();
  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
  }
  if (acquired_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(device_, acquired_, nullptr);
  }
  swapchain_ = VK_NULL_HANDLE;
  acquired_ = VK_NULL_HANDLE;
  device_ = VK_NULL_HANDLE;
}

} // namespace axiom::renderer
