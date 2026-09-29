# Module `renderer_present` — public API
Depends on: core, math, renderer_vk

## `modules/renderer_present/include/axiom/renderer_present/swapchain.hpp` — Swapchain for the window path (ROADMAP Stage 1): SRGB color output (CONVENTIONS §4), FIFO presentation,
- `struct SwapchainDesc`
  - `VkSurfaceKHR surface = VK_NULL_HANDLE`
  - `std::uint32_t width = 0`
  - `std::uint32_t height = 0`
  - `VkImageUsageFlags extraUsage = 0`
- `struct SwapchainFrame`
  /// One acquired image. The caller waits on `acquired` before color output and signals `renderFinished`.
  - `std::uint32_t index = 0`
  - `VkImage image = VK_NULL_HANDLE`
  - `VkImageView view = VK_NULL_HANDLE`
  - `VkSemaphore acquired = VK_NULL_HANDLE`
  - `VkSemaphore renderFinished = VK_NULL_HANDLE`
- `class Swapchain`
  /// @owns the VkSwapchainKHR, one SRGB view and one render-finished semaphore per image, and the
  ///       image-acquired semaphore; move-only. Does not own the surface.
  /// @lifetime must be destroyed before the device; the surface must outlive it.
  /// @thread single-threaded.
  /// @errors create()/recreate() fail with kUnsupported if the surface offers no SRGB format and the device has
  ///         no mutable swapchain format, kInvalidArgument for a zero extent, kGpu on any other Vulkan failure.
  - `(move-only, default-constructible)`
  - `[[nodiscard]] static core::Result<Swapchain> create(const VulkanDevice &device, const SwapchainDesc &desc)`
  - `[[nodiscard]] core::Result<bool> recreate(std::uint32_t width, std::uint32_t height)`
  - `[[nodiscard]] core::Result<std::optional<SwapchainFrame>> acquire()`
  - `[[nodiscard]] core::Result<bool> present(const SwapchainFrame &frame)`
  - `void recordBegin(VkCommandBuffer cmd, const SwapchainFrame &frame, const float (&clearLinear)[4]) const noexcept`
  - `void recordEnd(VkCommandBuffer cmd, const SwapchainFrame &frame, VkBuffer readback = VK_NULL_HANDLE) const noexcept`
  - `[[nodiscard]] VkFormat viewFormat() const noexcept`
  - `[[nodiscard]] VkFormat imageFormat() const noexcept`
  - `[[nodiscard]] VkExtent2D extent() const noexcept`
  - `[[nodiscard]] std::uint32_t imageCount() const noexcept`
  - `[[nodiscard]] core::Result<bool> build(std::uint32_t width, std::uint32_t height)`
  - `void destroyImages() noexcept`
  - `void reset() noexcept`
  - `VkPhysicalDevice physical_ = VK_NULL_HANDLE`
  - `VkDevice device_ = VK_NULL_HANDLE`
  - `VkQueue queue_ = VK_NULL_HANDLE`
  - `VkSurfaceKHR surface_ = VK_NULL_HANDLE`
  - `bool mutableFormat_ = false`
  - `VkImageUsageFlags extraUsage_ = 0`
  - `VkSwapchainKHR swapchain_ = VK_NULL_HANDLE`
  - `VkFormat imageFormat_ = VK_FORMAT_UNDEFINED`
  - `VkFormat viewFormat_ = VK_FORMAT_UNDEFINED`
  - `VkExtent2D extent_{}`
  - `std::vector<VkImage> images_`
  - `std::vector<VkImageView> views_`
  - `std::vector<VkSemaphore> renderFinished_`
  - `VkSemaphore acquired_ = VK_NULL_HANDLE`
