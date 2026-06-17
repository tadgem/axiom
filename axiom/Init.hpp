#pragma once
#include "SDL3/SDL.h"
#include <slang-rhi.h>
#include <memory>

namespace axm {
    struct InitResult {
        bool m_OK;

        SDL_Window*             m_Window;
        rhi::IDevice*           m_Device;
        rhi::ISurface*          m_Surface;
        rhi::ICommandQueue*     m_Queue;

        static InitResult BAD ();
    };

    namespace init {
        InitResult   Init();
        void         Quit(const InitResult& init);
    }

}