
add_library(axiom
    ${CMAKE_CURRENT_SOURCE_DIR}/Init.cpp
    ${IMGUI_INCLUDE_DIRS}/imgui.cpp
    ${IMGUI_INCLUDE_DIRS}/imgui_demo.cpp
    ${IMGUI_INCLUDE_DIRS}/imgui_draw.cpp
    ${IMGUI_INCLUDE_DIRS}/imgui_tables.cpp
    ${IMGUI_INCLUDE_DIRS}/imgui_widgets.cpp
    ${IMGUI_INCLUDE_DIRS}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_INCLUDE_DIRS}/backends/imgui_impl_slang_rhi.cpp
        Log.hpp
        Utils.hpp
        Prim.hpp
        Utils.cpp
)

target_link_libraries(axiom PUBLIC SDL3-static slang-rhi)

target_include_directories(axiom PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IMGUI_INCLUDE_DIRS}
        ${SLANG_INCLUDE_DIRS}
        ${SDL_INCLUDE_DIRS}
)