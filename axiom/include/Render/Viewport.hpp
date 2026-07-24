#pragma once
#include "../Core/Maths.hpp"
#include "SDL3/SDL_video.h"

namespace axm {
    struct Viewport
    {
        aml::Float2 m_Size;
    };

    namespace viewports {
        Viewport GetFullscreenViewport(SDL_Window* window);
    }

}
