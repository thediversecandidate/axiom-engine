# Helpers that apply CONVENTIONS §1–2 to every engine and test target.

function(axiom_warnings target)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endfunction()

# axiom_add_module(<name> SOURCES <files...> [DEPS <axiom::x ...>])
# Creates axiom_<name> (alias axiom::<name>) with the include root modules/<name>/include.
function(axiom_add_module name)
  cmake_parse_arguments(M "" "" "SOURCES;DEPS" ${ARGN})
  add_library(axiom_${name} STATIC ${M_SOURCES})
  add_library(axiom::${name} ALIAS axiom_${name})
  target_include_directories(axiom_${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(axiom_${name} PUBLIC cxx_std_23)
  target_compile_options(axiom_${name} PRIVATE -fno-exceptions -fno-rtti)
  if(M_DEPS)
    target_link_libraries(axiom_${name} PUBLIC ${M_DEPS})
  endif()
  axiom_warnings(axiom_${name})
endfunction()

# axiom_add_module_tests(<name> SOURCES <files...>) — Catch2 tests, built WITH exceptions.
function(axiom_add_module_tests name)
  cmake_parse_arguments(T "" "" "SOURCES" ${ARGN})
  add_executable(axiom_${name}_tests ${T_SOURCES})
  target_link_libraries(axiom_${name}_tests PRIVATE axiom::${name} Catch2::Catch2WithMain)
  target_compile_options(axiom_${name}_tests PRIVATE -fexceptions)
  axiom_warnings(axiom_${name}_tests)
  add_test(NAME ${name}.unit COMMAND axiom_${name}_tests)
endfunction()
