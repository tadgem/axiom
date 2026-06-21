
add_library(axiom
        ${IMGUI_INCLUDE_DIRS}/imgui.cpp
        ${IMGUI_INCLUDE_DIRS}/imgui_demo.cpp
        ${IMGUI_INCLUDE_DIRS}/imgui_draw.cpp
        ${IMGUI_INCLUDE_DIRS}/imgui_tables.cpp
        ${IMGUI_INCLUDE_DIRS}/imgui_widgets.cpp
        ${IMGUI_INCLUDE_DIRS}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_INCLUDE_DIRS}/backends/imgui_impl_slang_rhi.cpp
        Engine.cpp
        Log.hpp
        Utils.hpp
        Prim.hpp
        Utils.cpp
        Maths.hpp
        Axiom.hpp
)

target_link_libraries(axiom PUBLIC
        SDL3-static
        slang-rhi
        assimp
        flecs_static
)

target_include_directories(axiom PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IMGUI_INCLUDE_DIRS}
        ${SLANG_INCLUDE_DIRS}
        ${SDL_INCLUDE_DIRS}
)