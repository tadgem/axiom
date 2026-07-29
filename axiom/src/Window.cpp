#include "Core/Window.hpp"
bool axm::Window::GrabCursor() const {
    bool b = SDL_SetWindowMouseGrab(m_Window, true);
    b |= SDL_HideCursor();
    // Force mouse redraw
    SDL_SetCursor(nullptr);
    return b;
}
bool axm::Window::ReleaseCursor() const {
    bool b = SDL_SetWindowMouseGrab(m_Window, false);
    b |= SDL_ShowCursor();
    // Force mouse redraw
    SDL_SetCursor(nullptr);
    return b;
}
