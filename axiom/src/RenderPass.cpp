#include "../include/Render/RenderPass.hpp"

#include "Core/Profile.hpp"
rhi::IRenderPassEncoder* axm::render_pass::BeginSwapChainRenderPass(AppState&             app,
                                                                    rhi::ICommandEncoder* cmd,
                                                                    rhi::LoadOp           colourLoadOp,
                                                                    rhi::LoadOp           depthLoadOp,
                                                                    bool                  enableDepthTest,
                                                                    const vec4&           clearColour) {
    PROFILE_SCOPE()

    using namespace rhi;
    RenderPassColorAttachment colorAttachment        = { };
    colorAttachment.view                             = app.m_SwapchainColourImage->getDefaultView();
    colorAttachment.loadOp                           = colourLoadOp;
    colorAttachment.storeOp                          = StoreOp::Store;
    colorAttachment.clearValue[0]                    = clearColour._[0];
    colorAttachment.clearValue[1]                    = clearColour._[1];
    colorAttachment.clearValue[2]                    = clearColour._[2];
    colorAttachment.clearValue[3]                    = clearColour._[3];

    RenderPassDepthStencilAttachment depthAttachment = { };
    if (enableDepthTest) {
        depthAttachment.view            = app.m_SwapchainDepthImage->getDefaultView();
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
