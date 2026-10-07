file(GLOB_RECURSE AXM_SRCS src/*.cpp)

add_library(axiom STATIC
    ${THIRD_PARTY_DIRS}/imguizmo/ImGuizmo.cpp
    ${AXM_SRCS}
)

target_link_libraries(axiom PUBLIC
        vku
        vku-sdl
        SDL3-static
        assimp
        flecs_static
        mimalloc-static
        Jolt
        luajit::lib
)

target_include_directories(axiom PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUIZMO_INCLUDE_DIRS}
    ${SDL_INCLUDES}
    ${ASSIMP_INCLUDES}
    ${GLM_INCLUDES}
    ${MIMALLOC_INCLUDE_DIRS}
    ${SINGLE_HEADER_INCLUDE_DIRS}
    ${SUBMODULE_DIRS}/luajit-cmake/LuaJIT/src
)
