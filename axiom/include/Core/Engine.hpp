#pragma once
#include "Assets/AssetManager.hpp"
#include "Core/STL.hpp"
#include "Core/Timer.hpp"
#include "Core/Window.hpp"
#include "Render/GPU.hpp"
#include "SDL3/SDL.h"

namespace axm {
    struct AppState
    {
        bool            m_OK      = false;
        bool            m_Running = true;

        Timer           m_FrameTimer;
        f64             m_DeltaTime;

        AssetManager    m_AssetManager;


        Window          m_Window;
        GPU             m_GPU;

        static AppState BAD();
    };

    namespace engine {
        AppState Init();
        void     Quit(const AppState& e);
        void     PreFrame(AppState& e);
        void     PostFrame(AppState& e);
        void     OnWindowResized(AppState& e, SDL_Event& ev);

    } // namespace engine

} // namespace axm
