#pragma once
#include "Core/Engine.hpp"
#include "Core/Maths.hpp"

namespace axm {
    namespace render_pass {
        rhi::IRenderPassEncoder* BeginSwapChainRenderPass(AxiomEngine&             app,
                                                          rhi::ICommandEncoder* cmd,
                                                          rhi::LoadOp           colourLoadOp    = rhi::LoadOp::Clear,
                                                          rhi::LoadOp           depthLoadOp     = rhi::LoadOp::Clear,
                                                          bool                  enableDepthTest = true,
                                                          const aml::Vec4& clearColour = { 0.0f, 0.0f, 0.0f, 1.0f });
    };
} // namespace axm
