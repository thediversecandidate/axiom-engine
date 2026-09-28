# Axiom toolchain: Clang 21 + libstdc++ (GCC 15), x86-64-v3 (CONVENTIONS §1, ADR-0004).
# Applies the CPU baseline to ALL compilation, FetchContent code included. The standard library is
# the system libstdc++, the same one every Vulkan layer, driver and plugin on Linux uses.
#
# Local-only escape hatch: -DAXIOM_CLANG_VERSION=<major> -DAXIOM_ALLOW_UNPINNED_COMPILER=ON
# lets a machine without Clang 21 build for development. CI never sets it.

set(AXIOM_CLANG_VERSION "21" CACHE STRING "Pinned Clang major version")
set(AXIOM_ALLOW_UNPINNED_COMPILER OFF CACHE BOOL "Local-only: allow a Clang major other than 21")
# CMake's internal try_compile projects re-read this file; pass the version through to them.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES AXIOM_CLANG_VERSION)

find_program(AXIOM_CLANG_C NAMES clang-${AXIOM_CLANG_VERSION} REQUIRED)
find_program(AXIOM_CLANG_CXX NAMES clang++-${AXIOM_CLANG_VERSION} REQUIRED)

set(CMAKE_C_COMPILER "${AXIOM_CLANG_C}")
set(CMAKE_CXX_COMPILER "${AXIOM_CLANG_CXX}")

set(CMAKE_C_FLAGS_INIT "-march=x86-64-v3")
set(CMAKE_CXX_FLAGS_INIT "-stdlib=libstdc++ -march=x86-64-v3")
