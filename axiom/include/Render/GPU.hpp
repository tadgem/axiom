#pragma once

#include <slang-rhi.h>
#include "Render/Shader.hpp"

namespace axm {
    struct GPU
    {
        rhi::IDevice*                      m_Device               = nullptr;
        rhi::ISurface*                     m_Surface              = nullptr;
        rhi::ICommandQueue*                m_Queue                = nullptr;

        rhi::ITexture*                     m_SwapchainColourImage = nullptr;
        rhi::ITexture*                     m_SwapchainDepthImage  = nullptr;

        Unique<rhi::IDebugCallback>        m_DebugCallback;

        Shader                             m_MipShader;
        rhi::ComPtr<rhi::IComputePipeline> m_MipPipeline;
        rhi::ComPtr<rhi::ISampler>         m_LinearClampSampler;
    };
}
