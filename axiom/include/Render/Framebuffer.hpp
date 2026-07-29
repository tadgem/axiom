#pragma once
#include "Core/STL.hpp"
#include "Render/GPU.hpp"
#include "Render/RenderPass.hpp"
#include "Render/Texture.hpp"

namespace axm {
    class Framebuffer
    {
    public:
        Framebuffer(GPU& gpu);

        void                                           Resize(u32 newW, u32 newH);
        void                                           AddColourAttachment(rhi::Format        format,
                                                                           rhi::TextureUsage  usage,
                                                                           rhi::ResourceState defaultState,
                                                                           const char*        label = "UnknownAttachment");
        void                                           AddDepthAttachment(rhi::Format        format,
                                                                          rhi::TextureUsage  usage,
                                                                          rhi::ResourceState defaultState,
                                                                          const char*        label = "UnknownDepthAttachment");

        Array<Texture, render_pass::kMaxColourTargets> m_ColourAttachments;
        Texture                                        m_DepthStencilAttachment;


        GPU&                                           m_GPU;
        u32                                            m_Width, m_Height;
        u8                                             m_NumColourAttachments;
    };

}
