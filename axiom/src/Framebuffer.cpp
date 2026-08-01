#include "Render/Framebuffer.hpp"
axm::Framebuffer::Framebuffer(GPU& gpu) :
    m_ColourAttachments({ }), m_DepthStencilAttachment({ }), m_GPU(gpu),
    m_Width(gpu.m_SwapchainColourImage->getDesc().size.width),
    m_Height(gpu.m_SwapchainColourImage->getDesc().size.height), m_NumColourAttachments(0) { }
bool axm::Framebuffer::HasDepth() const { return m_DepthStencilAttachment.m_Texture.m_GPUTexture != nullptr; }

void axm::Framebuffer::Resize(u32 newW, u32 newH) {
    if (HasDepth()) {
        m_DepthStencilAttachment.m_Texture.m_GPUTexture->release();
        m_DepthStencilAttachment.m_Texture.m_TextureView = nullptr;

        AddDepthAttachment(m_DepthStencilAttachment.m_Texture.m_Format,
                           m_DepthStencilAttachment.m_Usage,
                           m_DepthStencilAttachment.m_DefaultState,
                           m_DepthStencilAttachment.m_Label);
    }
    const auto count       = m_NumColourAttachments;
    m_NumColourAttachments = 0;

    for (auto i = 0; i < count; i++) {
        m_ColourAttachments[i].m_Texture.m_GPUTexture->release();
        m_ColourAttachments[i].m_Texture.m_TextureView = nullptr;
        AddColourAttachment(m_ColourAttachments[i].m_Texture.m_Format,
                            m_ColourAttachments[i].m_Usage,
                            m_ColourAttachments[i].m_DefaultState,
                            m_ColourAttachments[i].m_Label);
    }
}

void axm::Framebuffer::AddColourAttachment(rhi::Format        format,
                                           rhi::TextureUsage  usage,
                                           rhi::ResourceState defaultState,
                                           const char*        label) {

    m_ColourAttachments[m_NumColourAttachments++]
            = { .m_Texture
                = textures::CreateRenderTexture2D(m_GPU, format, m_Width, m_Height, usage, defaultState, false, label),
                .m_Usage        = usage,
                .m_DefaultState = defaultState,
                .m_Label        = label };
}

void axm::Framebuffer::AddDepthAttachment(rhi::Format        format,
                                          rhi::TextureUsage  usage,
                                          rhi::ResourceState defaultState,
                                          const char*        label) {
    const auto renderTex
            = textures::CreateRenderTexture2D(m_GPU, format, m_Width, m_Height, usage, defaultState, false, label);
    m_DepthStencilAttachment
            = { .m_Texture = renderTex, .m_Usage = usage, .m_DefaultState = defaultState, .m_Label = label };
}
axm::DynArray<rhi::Format> axm::Framebuffer::GetFormatList() const {
    DynArray<rhi::Format> formats = { };
    for (auto i = 0; i < m_NumColourAttachments; i++) {
        const auto attachment = m_ColourAttachments[i];
        formats.push_back(attachment.m_Texture.m_Format);
    }

    return formats;
}

rhi::IRenderPassEncoder* axm::Framebuffer::BeginRenderPass(rhi::ICommandEncoder* cmd) const {
    render_pass::Builder b(cmd);

    if (HasDepth()) {
        b.AddDepthTarget(m_DepthStencilAttachment.m_Texture.m_TextureView, rhi::LoadOp::Clear);
    }

    for (auto i = 0; i < m_NumColourAttachments; i++) {
        const auto attachment = m_ColourAttachments[i];
        b.AddColourTarget(attachment.m_Texture.m_TextureView, rhi::LoadOp::Clear, aml::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    return b.Build();
}
