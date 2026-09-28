axiom_add_module(app SOURCES src/module_info.cpp DEPS axiom::core axiom::math axiom::physics axiom::renderer axiom::ai)
axiom_add_module_tests(app SOURCES tests/test_app.cpp)
