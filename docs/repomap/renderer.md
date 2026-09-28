# Module `renderer` — public API
Depends on: core, math

## `modules/renderer/include/axiom/renderer/module_info.hpp` — Stage 0 placeholder API for the renderer module. Allowed dependencies: axiom::core axiom::math.
- `[[nodiscard]] std::string_view moduleName() noexcept`
  /// Name of this module, used by the Stage 0 smoke test.

## `modules/renderer/include/axiom/renderer/validation.hpp` — Validation-message sink: counts Vulkan debug-utils messages by severity so tests can fail on
- `struct ValidationSink`
  /// @thread callbacks may arrive on any thread; all members are atomic.
  /// @lifetime the caller owns the sink; it must outlive every VulkanInstance that reports to it.
  - `std::atomic<std::uint32_t> errors{0}`
  - `std::atomic<std::uint32_t> warnings{0}`
  - `std::atomic<std::uint32_t> infos{0}`
  - `char firstError[512] = {}`
  - `std::atomic<bool> firstErrorSet{false}`

## `modules/renderer/include/axiom/renderer/vulkan_instance.hpp` — Vulkan 1.3 instance with the Khronos validation layer and a debug-utils messenger.
- `struct InstanceDesc`
  - `const char *appName = "axiom"`
  - `bool enableValidation = true`
  - `ValidationSink *sink = nullptr`
- `class VulkanInstance`
  /// @owns the VkInstance and its debug messenger; move-only.
  /// @lifetime destroy every device created from it first; desc.sink must outlive it.
  /// @errors create() fails with kUnsupported if Vulkan 1.3, the layer or the extension is missing.
  - `[[nodiscard]] static core::Result<VulkanInstance> create(const InstanceDesc &desc)`
  - `VulkanInstance() = default`
  - `VulkanInstance(VulkanInstance &&other) noexcept`
  - `VulkanInstance &operator=(VulkanInstance &&other) noexcept`
  - `VulkanInstance(const VulkanInstance &) = delete`
  - `VulkanInstance &operator=(const VulkanInstance &) = delete`
  - `~VulkanInstance()`
  - `[[nodiscard]] VkInstance handle() const noexcept`
  - `[[nodiscard]] bool validationEnabled() const noexcept`
  - `void submitTestMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, const char *text) const noexcept`
  - `void reset() noexcept`
  - `VkInstance instance_ = VK_NULL_HANDLE`
  - `VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE`
