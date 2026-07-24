#pragma once
#include "Core/Prim.hpp"
#include "SDL3/SDL.h"

namespace axm {
    class Window
    {
    public:
        SDL_Window* m_Window;

        u32         m_Width, m_Height;
    };
}
