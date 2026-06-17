#pragma once
#include "slang-rhi.h"
#include "Prim.hpp"

namespace axm {
    class Utils {
    public:
        static rhi::ITexture* CreateDepthTexture(rhi::IDevice* device, u32 w, u32 h, rhi::Format format = rhi::Format::D32Float);

    };
}