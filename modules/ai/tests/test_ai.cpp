#include "axiom/ai/module_info.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ai module links and reports its name", "[ai][stage0]") { CHECK(axiom::ai::moduleName() == "ai"); }
