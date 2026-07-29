#include "Render/Viewport.hpp"
#include "Core/Profile.hpp"

axm::Viewport axm::viewports::GetFullscreenViewport(SDL_Window* window) {
    PROFILE_SCOPE()
    i32 w, h;
    SDL_GetWindowSize(window, &w, &h);
    return { { CAST(w, f32), CAST(h, f32) } };
}
