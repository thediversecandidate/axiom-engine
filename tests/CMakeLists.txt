# Cross-module Stage 0 tests.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# One symbol from each single-header implementation library links and runs (ROADMAP Stage 0).
add_executable(axiom_third_party_tests test_third_party_link.cpp)
target_link_libraries(axiom_third_party_tests PRIVATE axiom_vma_impl axiom_cgltf_impl Catch2::Catch2WithMain)
axiom_warnings(axiom_third_party_tests)
add_test(NAME third_party.link COMMAND axiom_third_party_tests)

# Sanitizer detection fixtures: registered only in the asan-ubsan preset (CONVENTIONS §1).
if(AXIOM_SANITIZE)
  set(AXIOM_EXPECT ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/expect_failure.py)
  foreach(fixture ub_fixture ub_control leak_fixture leak_control)
    add_executable(axiom_${fixture} sanitizer/${fixture}.cpp)
    axiom_warnings(axiom_${fixture})
  endforeach()
  add_test(NAME sanitizer.ub_detected
    COMMAND ${AXIOM_EXPECT} --pattern "runtime error: signed integer overflow" -- $<TARGET_FILE:axiom_ub_fixture>)
  add_test(NAME sanitizer.leak_detected
    COMMAND ${AXIOM_EXPECT} --pattern "LeakSanitizer: detected memory leaks" -- $<TARGET_FILE:axiom_leak_fixture>)
  add_test(NAME sanitizer.ub_control COMMAND axiom_ub_control)
  add_test(NAME sanitizer.leak_control COMMAND axiom_leak_control)
  set_tests_properties(sanitizer.ub_detected sanitizer.leak_detected sanitizer.ub_control sanitizer.leak_control
    PROPERTIES ENVIRONMENT "LSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/lsan.supp")
endif()
