axiom_add_module(renderer SOURCES src/module_info.cpp DEPS axiom::core axiom::math)
axiom_add_module_tests(renderer SOURCES tests/test_renderer.cpp)
