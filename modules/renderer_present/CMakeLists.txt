# Presentation to a surface (swapchain), split from renderer_vk for the repo-map budget (ADR-0006).
axiom_add_module(renderer_present
  SOURCES src/swapchain.cpp src/swapchain_record.cpp
  DEPS axiom::core axiom::math axiom::renderer_vk)
axiom_add_module_tests(renderer_present SOURCES tests/test_swapchain.cpp)
axiom_lavapipe_tests(renderer_present.unit)
