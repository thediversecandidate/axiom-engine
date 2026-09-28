#include "axiom/physics/module_info.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("physics module links and reports its name", "[physics][stage0]") {
  CHECK(axiom::physics::moduleName() == "physics");
}
