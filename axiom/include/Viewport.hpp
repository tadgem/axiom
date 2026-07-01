#pragma once
#include "Maths.hpp"
#include "SDL3/SDL_video.h"

namespace axm {
    struct Viewport {
        vec2u   m_Size;
    };

    namespace viewports {
        Viewport GetFullscreenViewport(SDL_Window* window);
    }

}