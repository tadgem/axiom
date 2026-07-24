#include "Render/Viewport.hpp"
#include "Core/Profile.hpp"

axm::Viewport axm::viewports::GetFullscreenViewport(SDL_Window* window) {
    PROFILE_SCOPE()
    i32 w, h;
    SDL_GetWindowSize(window, &w, &h);
    return { { static_cast<f32>(w), static_cast<f32>(h) } };
}
