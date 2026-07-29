#include "../include/Render/RenderPass.hpp"

#include "Core/Profile.hpp"
rhi::IRenderPassEncoder* axm::render_pass::BeginSwapChainRenderPass(GPU&                  gpu,
                                                                    rhi::ICommandEncoder* cmd,
                                                                    rhi::LoadOp           colourLoadOp,
                                                                    rhi::LoadOp           depthLoadOp,
                                                                    bool                  enableDepthTest,
                                                                    const aml::Vec4&      clearColour) {
    PROFILE_SCOPE()

    Builder b(cmd);
    return b.AddColourTarget(gpu.m_SwapchainColourImage->getDefaultView(), colourLoadOp, clearColour)
            .AddDepthTarget(gpu.m_SwapchainDepthImage->getDefaultView(), depthLoadOp)
            .Build();
}
axm::render_pass::Builder::Builder(rhi::ICommandEncoder* cmd) : m_CommandEncoder(cmd) { }
axm::render_pass::Builder& axm::render_pass::Builder::AddColourTarget(rhi::ITextureView* texture,
                                                                      rhi::LoadOp        loadOp,
                                                                      const aml::Vec4&   clearColour) {
    PROFILE_SCOPE()
    AXM_ASSERT(m_NumColourTargets != kMaxColourTargets, "Cannot add another colour attachment.");

    m_ColourTargets[m_NumColourTargets].view          = texture;
    m_ColourTargets[m_NumColourTargets].loadOp        = loadOp;
    m_ColourTargets[m_NumColourTargets].storeOp       = rhi::StoreOp::Store;
    m_ColourTargets[m_NumColourTargets].clearValue[0] = clearColour.mF32[0];
    m_ColourTargets[m_NumColourTargets].clearValue[1] = clearColour.mF32[1];
    m_ColourTargets[m_NumColourTargets].clearValue[2] = clearColour.mF32[2];
    m_ColourTargets[m_NumColourTargets].clearValue[3] = clearColour.mF32[3];
    m_NumColourTargets++;
    return *this;
}
axm::render_pass::Builder& axm::render_pass::Builder::AddDepthTarget(rhi::ITextureView* texture, rhi::LoadOp loadOp) {
    m_DepthStencil.view            = texture;
    m_DepthStencil.depthLoadOp     = loadOp;
    m_DepthStencil.depthStoreOp    = rhi::StoreOp::Store;
    m_DepthStencil.depthClearValue = 1.0f;
    m_HasDepthStencil              = true;
    return *this;
}
axm::render_pass::Builder&
axm::render_pass::Builder::AddStencilTarget(rhi::LoadOp loadOp, rhi::StoreOp storeOp, u8 clearValue) {
    m_DepthStencil.stencilClearValue = clearValue;
    m_DepthStencil.stencilLoadOp     = loadOp;
    m_DepthStencil.stencilStoreOp    = storeOp;
    m_DepthStencil.stencilReadOnly   = false;
    return *this;
}
rhi::IRenderPassEncoder* axm::render_pass::Builder::Build() const {
    using namespace rhi;
    RenderPassDesc renderPass       = { };
    renderPass.colorAttachments     = &m_ColourTargets[0];
    renderPass.colorAttachmentCount = m_NumColourTargets;
    if (m_HasDepthStencil) {
        renderPass.depthStencilAttachment = &m_DepthStencil;
    } else {
        renderPass.depthStencilAttachment = nullptr;
    }

    return m_CommandEncoder->beginRenderPass(renderPass);
}
