#pragma once
#include "SDL3/SDL.h"
#include <slang-rhi.h>
#include <memory>

namespace axm {
    struct AppState {
        bool m_OK = false;
        bool m_Running = true;

        SDL_Window*             m_Window    = nullptr;
        rhi::IDevice*           m_Device    = nullptr;
        rhi::ISurface*          m_Surface   = nullptr;
        rhi::ICommandQueue*     m_Queue     = nullptr;

        rhi::ITexture*          m_SwapchainColourImage  = nullptr;
        rhi::ITexture*          m_SwapchainDepthImage   = nullptr;

        static AppState BAD ();

    };

    namespace engine {
        AppState    Init();
        void        Quit(const AppState& e);
        void        PreFrame(AppState& e);
        void        PostFrame(AppState& e);

        rhi::IRenderPassEncoder*    BeginSwapchainRenderPass(
            AppState& e,
            rhi::ICommandEncoder* cmd,
            rhi::LoadOp loadOp = rhi::LoadOp::Clear);
    }

}