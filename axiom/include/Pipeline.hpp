#pragma once
#include <span>
#include "Shader.hpp"
#include "slang-rhi.h"

namespace axm {
    namespace pipeline  {

        rhi::ComPtr<rhi::IRenderPipeline> CreateRasterPipeline (
            rhi::IDevice* device,
            const std::span<rhi::Format> &colourFormats,
            const rhi::DepthStencilDesc &depthTarget,
            const Shader& shader,
            rhi::IInputLayout* inputLayout
        );


    };
} // namespace axm
