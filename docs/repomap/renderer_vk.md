# Module `renderer_vk` — public API
Depends on: core, math

## `modules/renderer_vk/include/axiom/renderer_vk/command_context.hpp` — One command pool + command buffer + fence for synchronous submission (tests, offscreen renders).
- `class CommandContext`
  /// @owns a command pool, one primary command buffer and a fence; move-only.
  /// @lifetime must be destroyed before the device.
  /// @thread single-threaded: begin() and submitAndWait() on one thread.
  /// @errors begin()/submitAndWait() return kGpu on any Vulkan failure.
  - `(move-only, default-constructible)`
  - `[[nodiscard]] static core::Result<CommandContext> create(const VulkanDevice &device)`
  - `[[nodiscard]] core::Result<VkCommandBuffer> begin()`
  - `[[nodiscard]] core::Result<bool> submitAndWait()`
  - `void reset() noexcept`
  - `VkDevice device_ = VK_NULL_HANDLE`
  - `VkQueue queue_ = VK_NULL_HANDLE`
  - `VkCommandPool pool_ = VK_NULL_HANDLE`
  - `VkCommandBuffer cmd_ = VK_NULL_HANDLE`
  - `VkFence fence_ = VK_NULL_HANDLE`

## `modules/renderer_vk/include/axiom/renderer_vk/gpu_allocator.hpp` — GPU memory allocation through the Vulkan Memory Allocator (CONVENTIONS §1).
- `class GpuAllocator`
  /// @owns the VmaAllocator; move-only.
  /// @lifetime destroy every allocation made with it first; must be destroyed before the device.
  /// @errors create() fails with kGpu if vmaCreateAllocator fails.
  - `(move-only, default-constructible)`
  - `[[nodiscard]] static core::Result<GpuAllocator> create(const VulkanInstance &instance, const VulkanDevice &device)`
  - `[[nodiscard]] VmaAllocator handle() const noexcept`
  - `VmaAllocator allocator_ = VK_NULL_HANDLE`

## `modules/renderer_vk/include/axiom/renderer_vk/host_buffer.hpp` — Host-visible GPU buffer filled from CPU memory (vertex data, small uploads).
- `class HostBuffer`
  /// @owns a VMA buffer allocation; move-only.
  /// @lifetime must be destroyed before the GpuAllocator it was created with.
  /// @errors create() fails with kGpu if the allocation fails, kInvalidArgument for empty data.
  - `(move-only, default-constructible)`
  - `[[nodiscard]] static core::Result<HostBuffer> create(const GpuAllocator &allocator, VkBufferUsageFlags usage, std::span<const std::byte> data)`
  - `[[nodiscard]] VkBuffer handle() const noexcept`
  - `VmaAllocator allocator_ = VK_NULL_HANDLE`
  - `VkBuffer buffer_ = VK_NULL_HANDLE`
  - `VmaAllocation allocation_ = nullptr`

## `modules/renderer_vk/include/axiom/renderer_vk/validation.hpp` — Validation-message sink: counts Vulkan debug-utils messages by severity so tests can fail on
- `struct ValidationSink`
  /// @thread callbacks may arrive on any thread; all members are atomic.
  /// @lifetime the caller owns the sink; it must outlive every VulkanInstance that reports to it.
  - `std::atomic<std::uint32_t> errors{0}`
  - `std::atomic<std::uint32_t> warnings{0}`
  - `std::atomic<std::uint32_t> infos{0}`
  - `char firstError[512] = {}`
  - `std::atomic<bool> firstErrorSet{false}`

## `modules/renderer_vk/include/axiom/renderer_vk/vulkan_device.hpp` — Physical-device selection and logical-device creation with explicit Vulkan 1.3 feature enables.
- `struct DeviceDesc`
  - `std::optional<VkDriverId> requiredDriver`
- `class VulkanDevice`
  /// @owns the VkDevice; move-only.
  /// @lifetime must be destroyed before the VulkanInstance it was created from.
  /// @errors create() fails with kUnsupported if no device offers Vulkan 1.3, a graphics queue,
  ///         dynamicRendering and synchronization2 (and the required driver, if any).
  - `(move-only, default-constructible)`
  - `[[nodiscard]] static core::Result<VulkanDevice> create(const VulkanInstance &instance, const DeviceDesc &desc)`
  - `[[nodiscard]] VkDevice handle() const noexcept`
  - `[[nodiscard]] VkPhysicalDevice physical() const noexcept`
  - `[[nodiscard]] VkQueue graphicsQueue() const noexcept`
  - `[[nodiscard]] std::uint32_t graphicsQueueFamily() const noexcept`
  - `[[nodiscard]] VkDriverId driverId() const noexcept`
  - `void reset() noexcept`
  - `VkPhysicalDevice physical_ = VK_NULL_HANDLE`
  - `VkDevice device_ = VK_NULL_HANDLE`
  - `VkQueue queue_ = VK_NULL_HANDLE`
  - `std::uint32_t queueFamily_ = 0`
  - `VkDriverId driverId_ = static_cast<VkDriverId>(0)`

## `modules/renderer_vk/include/axiom/renderer_vk/vulkan_instance.hpp` — Vulkan 1.3 instance with the Khronos validation layer and a debug-utils messenger.
- `struct InstanceDesc`
  - `const char *appName = "axiom"`
  - `bool enableValidation = true`
  - `ValidationSink *sink = nullptr`
- `class VulkanInstance`
  /// @owns the VkInstance and its debug messenger; move-only.
  /// @lifetime destroy every device created from it first; desc.sink must outlive it.
  /// @errors create() fails with kUnsupported if Vulkan 1.3, the layer or the extension is missing.
  - `(move-only, default-constructible)`
  - `[[nodiscard]] static core::Result<VulkanInstance> create(const InstanceDesc &desc)`
  - `[[nodiscard]] VkInstance handle() const noexcept`
  - `[[nodiscard]] bool validationEnabled() const noexcept`
  - `void submitTestMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, const char *text) const noexcept`
  - `void reset() noexcept`
  - `VkInstance instance_ = VK_NULL_HANDLE`
  - `VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE`
