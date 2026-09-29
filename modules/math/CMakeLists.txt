axiom_add_module(math SOURCES src/scalar.cpp src/vector.cpp)
axiom_add_module_tests(math SOURCES tests/test_math.cpp tests/test_vector.cpp)

# normalized() asserts on invalid input in debug and asan-ubsan; in release the assert is compiled out.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_executable(axiom_math_normalize_zero tests/fixtures/normalize_zero.cpp)
target_link_libraries(axiom_math_normalize_zero PRIVATE axiom::math)
axiom_warnings(axiom_math_normalize_zero)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_test(NAME math.normalized_release_no_abort COMMAND axiom_math_normalize_zero)
else()
  add_test(NAME math.normalized_asserts
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/expect_failure.py
      --pattern "AXIOM_MATH_ASSERT failed: n.has_value\\(\\)" -- $<TARGET_FILE:axiom_math_normalize_zero>)
endif()
