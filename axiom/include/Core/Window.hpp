#pragma once
#include "Core/Prim.hpp"
#include "SDL3/SDL.h"

namespace axm {
    class Window
    {
    public:
        SDL_Window*     m_Window = nullptr;

        u32             m_Width = 0, m_Height = 0;

        NO_DISCARD bool GrabCursor() const;
        NO_DISCARD bool ReleaseCursor() const;
    };
}
