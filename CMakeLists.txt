cmake_minimum_required(VERSION 4.2)
project(axiom)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL3
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
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

# AXIOM
add_subdirectory(axiom)

add_executable(axiom-test main.cpp)
target_link_libraries(axiom-test PRIVATE axiom)
