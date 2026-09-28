axiom_add_module(renderer
  SOURCES src/module_info.cpp src/vulkan_instance.cpp src/vulkan_device.cpp src/gpu_allocator.cpp
    src/command_context.cpp src/offscreen_target.cpp
  DEPS axiom::core axiom::math Vulkan::Vulkan axiom_vma_impl)
axiom_add_module_tests(renderer SOURCES tests/test_renderer.cpp tests/test_vulkan_instance.cpp tests/test_vulkan_device.cpp
  tests/test_gpu_allocator.cpp tests/test_offscreen_target.cpp)

# Renderer tests run on lavapipe only (ROADMAP Stage 1). If the ICD is missing, the device tests
# fail (no silent pass): they require VK_DRIVER_ID_MESA_LLVMPIPE.
file(GLOB AXIOM_LAVAPIPE_ICD /usr/share/vulkan/icd.d/lvp_icd*.json)
if(AXIOM_LAVAPIPE_ICD)
  list(GET AXIOM_LAVAPIPE_ICD 0 AXIOM_LAVAPIPE_ICD)
  set_tests_properties(renderer.unit PROPERTIES
    ENVIRONMENT "VK_DRIVER_FILES=${AXIOM_LAVAPIPE_ICD};VK_ICD_FILENAMES=${AXIOM_LAVAPIPE_ICD}")
else()
  message(WARNING "lavapipe ICD not found: renderer device tests will fail")
endif()
