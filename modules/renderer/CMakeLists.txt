axiom_add_module(renderer
  SOURCES src/module_info.cpp src/offscreen_target.cpp src/shader_library.cpp src/triangle_pipeline.cpp
  DEPS axiom::core axiom::math axiom::renderer_vk)
axiom_add_module_tests(renderer SOURCES tests/test_renderer.cpp tests/test_offscreen_target.cpp
  tests/test_shader_library.cpp tests/test_triangle_draw.cpp
  tests/test_triangle_landmarks.cpp tests/test_triangle_culling.cpp
  tests/test_triangle_reference.cpp)
target_compile_definitions(axiom_renderer_tests PRIVATE AXIOM_GOLDEN_DIR="${PROJECT_SOURCE_DIR}/tests/golden"
  AXIOM_TEST_OUTPUT_DIR="${CMAKE_BINARY_DIR}/test_output")
axiom_add_shaders(axiom_renderer shaders/triangle.vert shaders/triangle.frag)

# Second run in the same environment: fails if any validation-layer state persisted across runs
# (e.g. the on-disk shader-validation cache) weakens the "zero validation errors" checks.
add_test(NAME renderer.unit.second_run COMMAND axiom_renderer_tests)
set_tests_properties(renderer.unit.second_run PROPERTIES DEPENDS renderer.unit)
axiom_lavapipe_tests(renderer.unit renderer.unit.second_run)
