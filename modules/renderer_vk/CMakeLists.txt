axiom_add_module(renderer_vk
  SOURCES src/vulkan_instance.cpp src/vulkan_device.cpp src/gpu_allocator.cpp src/command_context.cpp
    src/host_buffer.cpp
  DEPS axiom::core axiom::math Vulkan::Vulkan axiom_vma_impl)
axiom_add_module_tests(renderer_vk SOURCES tests/test_vulkan_instance.cpp tests/test_vulkan_device.cpp
  tests/test_gpu_allocator.cpp)
axiom_lavapipe_tests(renderer_vk.unit)
