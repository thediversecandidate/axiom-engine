axiom_add_module(renderer SOURCES src/module_info.cpp src/vulkan_instance.cpp DEPS axiom::core axiom::math Vulkan::Vulkan)
axiom_add_module_tests(renderer SOURCES tests/test_renderer.cpp tests/test_vulkan_instance.cpp)
