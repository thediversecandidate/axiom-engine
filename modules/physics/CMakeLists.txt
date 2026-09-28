axiom_add_module(physics SOURCES src/module_info.cpp DEPS axiom::core axiom::math)
axiom_add_module_tests(physics SOURCES tests/test_physics.cpp)
