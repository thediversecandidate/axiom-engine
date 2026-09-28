# Pinned third-party dependencies (CONVENTIONS §1). Every declare pins a full commit hash and uses SYSTEM.
find_package(Git REQUIRED)
find_package(Vulkan REQUIRED)
include(FetchContent)

FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG fd79eadb5bc1760e7cbae12fd45b0d0040d1bb73 # v3.16.0
  SYSTEM)

# Header-only libraries: SOURCE_SUBDIR points at a directory without CMakeLists.txt,
# so FetchContent only downloads them; Axiom defines its own targets below.
FetchContent_Declare(VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG 3aa921224c154a0d2c43912bc88e1c42ce1f7607 # v3.4.0
  SOURCE_SUBDIR axiom-download-only
  SYSTEM)

FetchContent_Declare(cgltf
  GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
  GIT_TAG bbeb5b0b070ddacddac6852fb72143eb68454937 # v1.15
  SOURCE_SUBDIR axiom-download-only
  SYSTEM)

# SDL3 is declared now and made available in Stage 1 (renderer window path).
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG fa2c02bb6e21974a89ea9824bc53c9932abe5f9c # release-3.4.16
  SYSTEM)

FetchContent_MakeAvailable(Catch2 VulkanMemoryAllocator cgltf)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_library(axiom_vma_headers INTERFACE)
target_include_directories(axiom_vma_headers SYSTEM INTERFACE ${vulkanmemoryallocator_SOURCE_DIR}/include)
target_link_libraries(axiom_vma_headers INTERFACE Vulkan::Vulkan)

add_library(axiom_cgltf_headers INTERFACE)
target_include_directories(axiom_cgltf_headers SYSTEM INTERFACE ${cgltf_SOURCE_DIR})
