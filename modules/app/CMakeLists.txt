axiom_add_module(app SOURCES src/module_info.cpp DEPS axiom::core axiom::math axiom::physics axiom::renderer axiom::ai)
axiom_add_module_tests(app SOURCES tests/test_app.cpp)

# Stage 1 window path. Engine rules apply (no exceptions/RTTI). The smoke test runs 3 frames with SDL's
# offscreen driver (VK_EXT_headless_surface) on lavapipe with validation; a real window is local only.
add_executable(axiom_triangle src/triangle_main.cpp)
target_link_libraries(axiom_triangle PRIVATE axiom::renderer axiom::renderer_present SDL3::SDL3)
target_compile_options(axiom_triangle PRIVATE -fno-exceptions -fno-rtti)
axiom_warnings(axiom_triangle)
add_test(NAME app.triangle_smoke COMMAND axiom_triangle --frames 3 --validation)
axiom_lavapipe_tests(app.triangle_smoke)
set_property(TEST app.triangle_smoke APPEND PROPERTY ENVIRONMENT SDL_VIDEO_DRIVER=offscreen)
