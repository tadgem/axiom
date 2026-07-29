#include "Core/Window.hpp"

bool axm::Window::GrabCursor() const {
    bool ok = SDL_SetWindowRelativeMouseMode(m_Window, true);
    ok &= SDL_HideCursor();
    return ok;
}

bool axm::Window::ReleaseCursor() const {
    bool ok = SDL_SetWindowRelativeMouseMode(m_Window, false);
    ok &= SDL_ShowCursor();
    return ok;
}
