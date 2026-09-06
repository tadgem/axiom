cmake_minimum_required(VERSION 4.2)
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

# SDL3
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl/include CACHE INTERNAL "")
add_subdirectory(third_party/sdl)

# Slang RHI
# slang-rhi backends not used by the engine.
set(SLANG_RHI_ENABLE_WGPU OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_ENABLE_CUDA OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_ENABLE_OPTIX OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_ENABLE_AGILITY_SDK OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_ENABLE_NVAPI OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/submodules/slang-rhi/include)
add_subdirectory(submodules/slang-rhi)

# ImGui
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/imguizmo)

# Flecs
set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
set(FLECS_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(submodules/flecs)

# ASSIMP
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable ASSIMP dynamic linking" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable ASSIMP tests (removed the dir)")
add_subdirectory(third_party/assimp)

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
