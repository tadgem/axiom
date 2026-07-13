#include "Render/Viewport.hpp"
#include "Core/Profile.hpp"

axm::Viewport axm::viewports::GetFullscreenViewport(SDL_Window* window) {
    PROFILE_SCOPE()
    i32 w, h;
    SDL_GetWindowSize(window, &w, &h);
    return { { .x = static_cast<u32>(w), .y = static_cast<u32>(h) } };
}
