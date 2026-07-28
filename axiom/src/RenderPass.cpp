#include "../include/Render/RenderPass.hpp"

#include "Core/Profile.hpp"
rhi::IRenderPassEncoder* axm::render_pass::BeginSwapChainRenderPass(GPU&                  gpu,
                                                                    rhi::ICommandEncoder* cmd,
                                                                    rhi::LoadOp           colourLoadOp,
                                                                    rhi::LoadOp           depthLoadOp,
                                                                    bool                  enableDepthTest,
                                                                    const aml::Vec4&      clearColour) {
    PROFILE_SCOPE()

    using namespace rhi;
    RenderPassColorAttachment colorAttachment        = { };
    colorAttachment.view                             = gpu.m_SwapchainColourImage->getDefaultView();
    colorAttachment.loadOp                           = colourLoadOp;
    colorAttachment.storeOp                          = StoreOp::Store;
    colorAttachment.clearValue[0]                    = clearColour.mF32[0];
    colorAttachment.clearValue[1]                    = clearColour.mF32[1];
    colorAttachment.clearValue[2]                    = clearColour.mF32[2];
    colorAttachment.clearValue[3]                    = clearColour.mF32[3];

    RenderPassDepthStencilAttachment depthAttachment = { };
    if (enableDepthTest) {
        depthAttachment.view            = gpu.m_SwapchainDepthImage->getDefaultView();
        depthAttachment.depthLoadOp     = depthLoadOp;
        depthAttachment.depthStoreOp    = StoreOp::Store;
        depthAttachment.depthClearValue = 1.0f;
    }

    RenderPassDesc renderPass       = { };
    renderPass.colorAttachments     = &colorAttachment;
    renderPass.colorAttachmentCount = 1;
    if (enableDepthTest) {
        renderPass.depthStencilAttachment = &depthAttachment;
    } else {
        renderPass.depthStencilAttachment = nullptr;
    }

    return cmd->beginRenderPass(renderPass);
}
