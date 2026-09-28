axiom_add_module(ai SOURCES src/module_info.cpp DEPS axiom::core axiom::math)
axiom_add_module_tests(ai SOURCES tests/test_ai.cpp)
