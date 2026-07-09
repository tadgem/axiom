#pragma once
#include <memory>
#include <slang-rhi.h>
#include "Assets/AssetManager.hpp"
#include "Core/Timer.hpp"
#include "SDL3/SDL.h"
#include "STL.hpp"

namespace axm {
    struct AppState
    {
        bool                        m_OK      = false;
        bool                        m_Running = true;

        Timer                       m_FrameTimer;
        f64                         m_DeltaTime;

        AssetManager                m_AssetManager;

        rhi::DepthStencilDesc       m_DepthStencilDesc;

        SDL_Window*                 m_Window               = nullptr;
        rhi::IDevice*               m_Device               = nullptr;
        rhi::ISurface*              m_Surface              = nullptr;
        rhi::ICommandQueue*         m_Queue                = nullptr;

        rhi::ITexture*              m_SwapchainColourImage = nullptr;
        rhi::ITexture*              m_SwapchainDepthImage  = nullptr;

        Unique<rhi::IDebugCallback> m_DebugCallback;

        static AppState             BAD();
    };

    namespace engine {
        AppState Init();
        void     Quit(const AppState& e);
        void     PreFrame(AppState& e);
        void     PostFrame(AppState& e);

        rhi::IRenderPassEncoder*
        BeginSwapchainRenderPass(AppState& e, rhi::ICommandEncoder* cmd, rhi::LoadOp loadOp = rhi::LoadOp::Clear);
    } // namespace engine

} // namespace axm
