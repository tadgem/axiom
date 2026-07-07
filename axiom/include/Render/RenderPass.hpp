#pragma once
#include "Core/Engine.hpp"
#include "Core/Maths.hpp"

namespace axm {
    namespace render_pass {
        rhi::IRenderPassEncoder* BeginSwapChainRenderPass(AppState& app,
                                                          rhi::ICommandEncoder* cmd,
                                                          const vec4& clearColour = { 0.0f, 0.0f, 0.0f, 1.0f });
    };
} // namespace axm
