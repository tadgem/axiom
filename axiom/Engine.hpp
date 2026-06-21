#pragma once
#include "SDL3/SDL.h"
#include <slang-rhi.h>
#include <memory>

namespace axm {
    struct AppState {
        bool m_OK = false;

        SDL_Window*             m_Window    = nullptr;
        rhi::IDevice*           m_Device    = nullptr;
        rhi::ISurface*          m_Surface   = nullptr;
        rhi::ICommandQueue*     m_Queue     = nullptr;

        static AppState BAD ();
    };

    namespace engine {
        AppState    Init();
        void        Quit(const AppState& init);
    }

}