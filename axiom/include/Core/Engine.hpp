#pragma once
#include "Assets/AssetManager.hpp"
#include "Core/STL.hpp"
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

        static AxiomEngine BAD();
    };

    namespace engine {
        AxiomEngine Init();
        void        Quit(const AxiomEngine& e);
        void        PreFrame(AxiomEngine& e);
        void        PostFrame(AxiomEngine& e);
        void        OnWindowResized(AxiomEngine& e, SDL_Event& ev);

    } // namespace engine

} // namespace axm
