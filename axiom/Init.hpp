#pragma once
#include "SDL3/SDL.h"
#include <slang-rhi.h>
#include <memory>

namespace axm {
    struct AppState {
        bool m_OK;

        SDL_Window*             m_Window;
        rhi::IDevice*           m_Device;
        rhi::ISurface*          m_Surface;
        rhi::ICommandQueue*     m_Queue;

        static AppState BAD ();
    };

    namespace init {
        AppState   Init();
        void         Quit(const AppState& init);
    }

}