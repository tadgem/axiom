#pragma once
#include "Core/STL.hpp"
#include "Render/GPU.hpp"
#include "Render/RenderPass.hpp"
#include "Render/Texture.hpp"

namespace axm {
    class Framebuffer
    {
    public:
        struct Attachment
        {
            Texture            m_Texture      = Texture::BAD();
            rhi::TextureUsage  m_Usage        = rhi::TextureUsage::None;
            rhi::ResourceState m_DefaultState = rhi::ResourceState::Undefined;
            const char*        m_Label        = "Unknown";
        };
        Framebuffer(GPU& gpu);

        NO_DISCARD bool HasDepth() const;

        void            Resize(u32 newW, u32 newH);
        void            AddColourAttachment(rhi::Format        format,
                                            rhi::TextureUsage  usage,
                                            rhi::ResourceState defaultState,
                                            const char*        label = "UnknownAttachment");
        void            AddDepthAttachment(rhi::Format        format,
                                           rhi::TextureUsage  usage,
                                           rhi::ResourceState defaultState,
                                           const char*        label = "UnknownDepthAttachment");

        NO_DISCARD DynArray<rhi::Format>                  GetFormatList() const;

        rhi::IRenderPassEncoder*                          BeginRenderPass(rhi::ICommandEncoder* cmd) const;
        Array<Attachment, render_pass::kMaxColourTargets> m_ColourAttachments;
        Attachment                                        m_DepthStencilAttachment;


        GPU&                                              m_GPU;
        u32                                               m_Width, m_Height;
        u8                                                m_NumColourAttachments;
    };

}
