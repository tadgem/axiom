#pragma once
#include "Assets/AssetManager.hpp"
#include "Core/Input.hpp"
#include "Core/Timer.hpp"
#include "Core/Window.hpp"
#include "Render/GPU.hpp"
#include "SDL3/SDL.h"

namespace axm {
    struct AxiomEngine
    {
        bool               m_OK      = false;
        bool               m_Running = true;
        Timer              m_FrameTimer;
        f64                m_DeltaTime;

        AssetManager       m_AssetManager;
        Window             m_Window;
        GPU                m_GPU;
        Input              m_Input;

        static AxiomEngine BAD();
        static AxiomEngine Init();

        void               Quit();
        NO_DISCARD bool    PreFrame();
        void               PostFrame();
        void               OnWindowResized(SDL_Event& ev);
    };

} // namespace axm
