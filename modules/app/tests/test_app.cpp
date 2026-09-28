#include "axiom/app/module_info.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("app module links and reports its name", "[app][stage0]") { CHECK(axiom::app::moduleName() == "app"); }
