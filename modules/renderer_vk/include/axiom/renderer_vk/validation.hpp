#pragma once
// Validation-message sink: counts Vulkan debug-utils messages by severity so tests can fail on
// any ERROR, including messages emitted during instance destruction (CONVENTIONS §4).

#include <atomic>
#include <cstdint>

namespace axiom::renderer {

/// @thread callbacks may arrive on any thread; all members are atomic.
/// @lifetime the caller owns the sink; it must outlive every VulkanInstance that reports to it.
struct ValidationSink {
  std::atomic<std::uint32_t> errors{0};
  std::atomic<std::uint32_t> warnings{0};
  std::atomic<std::uint32_t> infos{0};
  /// First ERROR message, NUL-terminated; written once.
  char firstError[512] = {};
  std::atomic<bool> firstErrorSet{false};
};

} // namespace axiom::renderer
