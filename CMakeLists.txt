cmake_minimum_required(VERSION 3.18)
project(axiom)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

message("-- Axiom : Configuring build --")

# mimalloc
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MIMALLOC_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mimalloc/include)
add_subdirectory(third_party/mimalloc)

# vku (Vulkan helper library) - provides SDL3, assimp, glm, volk, imgui, nanovg,
# im3d and runtime GLSL compilation via shaderc. Slang is disabled and vku's
# own examples are not built as part of axiom.
set(VKU_ENABLE_SLANG OFF CACHE BOOL "" FORCE)
set(VKU_RT_SHADER_COMPILATION ON CACHE BOOL "" FORCE)
set(VKU_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(submodules/vku)

# ImGuiZmo (the rest of the ImGui stack comes from vku).
add_subdirectory(third_party/imguizmo)

# Flecs
set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
set(FLECS_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(submodules/flecs)

# Jolt
set(JPH_BUILD_SHARED_LIBS OFF CACHE BOOL "Disable Jolt shared libs" FORCE)
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "Disable Jolt overriding CXX flags" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "Disable Jolt static MSVC runtime library" FORCE)
add_subdirectory(submodules/jolt/Build)

# Third Party Single Headers
set(THIRD_PARTY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(SINGLE_HEADER_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/single_headers)

# AXIOM
include(cmake/AxiomUtils.cmake)
add_subdirectory(axiom)

# TODO : Allow tests and examples to be defined in subdirectories
# issue with required DLLs not being copied to correct directory

option(AXIOM_BUILD_TESTS "Build Axiom tests" ${PROJECT_IS_TOP_LEVEL})
option(AXIOM_BUILD_EXAMPLES "Build Axiom examples" ${PROJECT_IS_TOP_LEVEL})

if(AXIOM_BUILD_TESTS)
    add_axiom_test(axiom-tests-assets tests/AssetTests.cpp)
    add_axiom_test(axiom-tests-engine tests/EngineTests.cpp)
endif()

if(AXIOM_BUILD_EXAMPLES)
    add_axiom_example(axiom-example-deferred examples/deferred/main.cpp)
    add_axiom_example(axiom-example-forward examples/forward/main.cpp)
endif()
