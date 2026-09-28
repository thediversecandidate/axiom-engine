# ADR-0004: Use libstdc++, not libc++

**Status:** accepted (2026-09-28). Supersedes the libc++ choice in ADR-0001 and CONVENTIONS §1.

## Context
Stage 1 step 2b ran the first GPU submission under the `asan-ubsan` preset. AddressSanitizer reported `alloc-dealloc-mismatch (operator new vs free)` inside `vkQueueSubmit2`:
* The Khronos validation layer (linked against libstdc++) internally creates and catches a `std::invalid_argument`. Its message is allocated by libstdc++ (`std::logic_error::logic_error`, `operator new`).
* The destructor that ran was `std::invalid_argument::~invalid_argument` from **libc++abi**, which frees with `free()` and assumes a different string layout.
* Cause: libc++abi and libstdc++ both export the same unversioned symbols (`_ZNSt11logic_errorD1Ev`, `_ZNSt16invalid_argumentD1Ev`, …; checked with `nm -D`). The dynamic linker binds every caller to the first definition, which is libc++abi, because it loads with our executable.
* The validation layer and lavapipe (through libLLVM) both link libstdc++ (checked with `ldd`), and so do ONNX Runtime's prebuilt binaries. Any exception thrown inside them would be destroyed by the wrong code. Outside ASan this is silent memory corruption.

## Decision
Use the system libstdc++ (GCC 15, `libstdc++-15-dev` on Ubuntu 26.04) with Clang 21. libc++ is forbidden. `std::expected`, the original reason for libc++, has been in libstdc++ since GCC 12. The Stage 0 build test now asserts that engine and tests see the same `__GLIBCXX__` and never `_LIBCPP_VERSION`.

## Consequences
One C++ runtime per process, shared with every Vulkan layer, driver and plugin. The `asan-ubsan` preset exercises real GPU submissions from Stage 1 on, so a regression would reappear as an ASan failure.
* libstdc++ enables `std::expected` only when `__cpp_concepts >= 202002L` (checked in `/usr/include/c++/13/expected`). Clang 18 reports `201907L`, so the local-only compiler override (ADR-0002) now needs Clang 19 or later. CI's Clang 21 is unaffected.
