#pragma once
#include "Core/Prim.hpp"
#include "SDL3/SDL.h"

namespace axm {
    class Window
    {
    public:
        SDL_Window*     m_Window;

        u32             m_Width, m_Height;

        NO_DISCARD bool GrabCursor() const;
        NO_DISCARD bool ReleaseCursor() const;
    };
}
