#include "Render/Framebuffer.hpp"
axm::Framebuffer::Framebuffer(GPU& gpu) :
    m_ColourAttachments({ }), m_DepthStencilAttachment({ }), m_GPU(gpu),
    m_Width(gpu.m_SwapchainColourImage->getDesc().size.width),
    m_Height(gpu.m_SwapchainColourImage->getDesc().size.height), m_NumColourAttachments(0) { }

void axm::Framebuffer::AddColourAttachment(rhi::Format        format,
                                           rhi::TextureUsage  usage,
                                           rhi::ResourceState defaultState,
                                           const char*        label) {

    m_ColourAttachments[m_NumColourAttachments++]
            = textures::CreateRenderTexture2D(m_GPU, format, m_Width, m_Height, usage, defaultState, false, label);
}
void axm::Framebuffer::AddDepthAttachment(rhi::Format        format,
                                          rhi::TextureUsage  usage,
                                          rhi::ResourceState defaultState,
                                          const char*        label) {
    m_DepthStencilAttachment
            = textures::CreateRenderTexture2D(m_GPU, format, m_Width, m_Height, usage, defaultState, false, label);
}
