file(GLOB_RECURSE AXM_SRCS src/*.cpp)

file(GLOB_RECURSE IMGUI_SRCS ${IMGUI_INCLUDE_DIRS}/*.cpp)
file(GLOB_RECURSE IMGUI_BACKEND_SRCS ${IMGUI_INCLUDE_DIRS}/backends/*.cpp)
file(GLOB_RECURSE IMGUIZMO_SRCS ${IMGUIZMO_INCLUDE_DIRS}/*.cpp)

add_library(axiom STATIC
    ${IMGUI_SRCS}
    ${IMGUI_BACKEND_SRCS}
    ${IMGUIZMO_SRCS}
    ${THIRD_PARTY_DIRS}/im3d/im3d.cpp
    ${THIRD_PARTY_DIRS}/im3d/backends/im3d_vk.cpp
    ${SUBMODULE_DIRS}/nanovg-vk/src/nanovg.c
    ${SUBMODULE_DIRS}/nanovg-vk/src/nanovg_vk.c
    ${AXM_SRCS}
)

target_link_libraries(axiom PUBLIC
        SDL3-static
        assimp
        flecs_static
        mimalloc-static
        Jolt
        volk
        spirv-reflect-static
)

target_include_directories(axiom PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUI_INCLUDE_DIRS}
    ${IMGUIZMO_INCLUDE_DIRS}
    ${SLANG_INCLUDE_DIRS}
    ${SDL_INCLUDE_DIRS}
    ${MIMALLOC_INCLUDE_DIRS}
    ${SINGLE_HEADER_INCLUDE_DIRS}
    ${THIRD_PARTY_DIRS}/im3d
    ${THIRD_PARTY_DIRS}/nanovg
    ${THIRD_PARTY_DIRS}/im3d/backends
    ${SUBMODULE_DIRS}/nanovg-vk/src
)