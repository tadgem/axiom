#include "Pipeline.hpp"
#include <vector>
#include "Debug.hpp"

rhi::ComPtr<rhi::IRenderPipeline> axm::pipeline::CreateRasterPipeline(rhi::IDevice *device,
                                                                      const Span<rhi::Format> &colourFormats,
                                                                      const rhi::DepthStencilDesc &depthTarget,
                                                                      const Shader &shader,
                                                                      rhi::IInputLayout *inputLayout) {

    Vector<rhi::ColorTargetDesc> colorTargets;
    colorTargets.resize(colourFormats.size());

    for (auto i = 0; i < colourFormats.size(); i++) {
        colorTargets[i].format = colourFormats[i];
    }

    rhi::RenderPipelineDesc pipelineDesc = {};
    pipelineDesc.program = shader.m_Program;
    pipelineDesc.inputLayout = inputLayout;
    pipelineDesc.targets = colorTargets.data();
    pipelineDesc.targetCount = colorTargets.size();
    pipelineDesc.depthStencil = depthTarget;
    pipelineDesc.label = shader.m_Name;
    rhi::ComPtr<rhi::IRenderPipeline> pipeline;

    if (SLANG_FAILED(device->createRenderPipeline(pipelineDesc, pipeline.writeRef()))) {
        AXM_LOG("Failed to create render pipeline with shader : {}", shader.m_Name);
        return {};
    }

    return pipeline;
}
