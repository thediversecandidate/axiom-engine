# Exactly one implementation translation unit per single-header library (CONVENTIONS §1).
# These are third-party code: no -Werror, but the same toolchain flags (libstdc++, x86-64-v3).

add_library(axiom_vma_impl STATIC vma_impl.cpp)
target_link_libraries(axiom_vma_impl PUBLIC axiom_vma_headers)
target_compile_features(axiom_vma_impl PUBLIC cxx_std_23)

add_library(axiom_cgltf_impl STATIC cgltf_impl.cpp)
target_link_libraries(axiom_cgltf_impl PUBLIC axiom_cgltf_headers)
