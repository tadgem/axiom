#pragma once
#include <memory>
#include "Assets/AssetManager.hpp"
#include "Core/Timer.hpp"
#include "Render/GPU.hpp"
#include "SDL3/SDL.h"
#include "STL.hpp"

namespace axm {
    struct AppState
    {
        bool                  m_OK      = false;
        bool                  m_Running = true;

        Timer                 m_FrameTimer;
        f64                   m_DeltaTime;

        AssetManager          m_AssetManager;

        rhi::DepthStencilDesc m_DepthStencilDesc;

        SDL_Window*           m_Window = nullptr;
        GPU                   m_GPU;

        static AppState       BAD();
    };

    namespace engine {
        AppState Init();
        void     Quit(const AppState& e);
        void     PreFrame(AppState& e);
        void     PostFrame(AppState& e);

    } // namespace engine

} // namespace axm
