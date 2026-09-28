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

# SDL3 (zlib license): window + Vulkan surface for the local window path (Stage 1). Static, video only.
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG fa2c02bb6e21974a89ea9824bc53c9932abe5f9c # release-3.4.16
  SYSTEM)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
foreach(subsystem AUDIO CAMERA JOYSTICK HAPTIC HIDAPI SENSOR POWER RENDER GPU DIALOG)
  set(SDL_${subsystem} OFF CACHE BOOL "" FORCE)
endforeach()
# Optional X11 extensions are not needed for a window + Vulkan surface; SDL errors if one is only partly
# installed, so they are off. Plain X11 and Wayland windows are unaffected.
foreach(x11ext XCURSOR XDBE XINPUT XFIXES XRANDR XSCRNSAVER XSHAPE XSYNC XTEST)
  set(SDL_X11_${x11ext} OFF CACHE BOOL "" FORCE)
endforeach()
set(SDL_VULKAN ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(Catch2 VulkanMemoryAllocator cgltf SDL3)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_library(axiom_vma_headers INTERFACE)
target_include_directories(axiom_vma_headers SYSTEM INTERFACE ${vulkanmemoryallocator_SOURCE_DIR}/include)
target_link_libraries(axiom_vma_headers INTERFACE Vulkan::Vulkan)

add_library(axiom_cgltf_headers INTERFACE)
target_include_directories(axiom_cgltf_headers SYSTEM INTERFACE ${cgltf_SOURCE_DIR})
