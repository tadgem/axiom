#include "RenderPass.hpp"
rhi::IRenderPassEncoder *axm::render_pass::BeginSwapChainRenderPass(AppState &app, rhi::ICommandEncoder *cmd,
                                                                    const vec4 &clearColour) {
    using namespace rhi;
    RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = app.m_SwapchainColourImage->getDefaultView();
    colorAttachment.loadOp = LoadOp::Clear;
    colorAttachment.storeOp = StoreOp::Store;
    colorAttachment.clearValue[0] = clearColour._[0];
    colorAttachment.clearValue[1] = clearColour._[1];
    colorAttachment.clearValue[2] = clearColour._[2];
    colorAttachment.clearValue[3] = clearColour._[3];

    RenderPassDepthStencilAttachment depthAttachment = {};
    depthAttachment.view = app.m_SwapchainDepthImage->getDefaultView();
    depthAttachment.depthLoadOp = LoadOp::Clear;
    depthAttachment.depthStoreOp = StoreOp::Store;
    depthAttachment.depthClearValue = 1.0f;

    RenderPassDesc renderPass = {};
    renderPass.colorAttachments = &colorAttachment;
    renderPass.colorAttachmentCount = 1;
    renderPass.depthStencilAttachment = &depthAttachment;

    return cmd->beginRenderPass(renderPass);
}
