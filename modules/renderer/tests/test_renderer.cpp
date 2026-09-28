#include "axiom/renderer/module_info.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("renderer module links and reports its name", "[renderer][stage0]") {
  CHECK(axiom::renderer::moduleName() == "renderer");
}
