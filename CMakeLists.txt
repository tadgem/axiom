cmake_minimum_required(VERSION 4.2)
project(axiom)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("-- Axiom : Configuring build --")

# mimalloc
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MIMALLOC_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mimalloc/include)
add_subdirectory(third_party/mimalloc)

# SDL3
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl/include CACHE INTERNAL "")
add_subdirectory(third_party/sdl)

# Slang RHI
set(SLANG_RHI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SLANG_RHI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/submodules/slang-rhi/include)
add_subdirectory(submodules/slang-rhi)

# ImGui
add_subdirectory(third_party/imgui)

# Flecs
set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
set(FLECS_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(submodules/flecs)

# ASSIMP
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable ASSIMP dynamic linking")
add_subdirectory(submodules/assimp)

# Third Party Single Headers
set(SINGLE_HEADER_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/single_headers)

# AXIOM
add_subdirectory(axiom)
include(cmake/AxiomUtils.cmake)

add_axiom_target(axiom-test-basic examples/basic/main.cpp)
add_axiom_target(axiom-test-deferred examples/deferred/main.cpp)
