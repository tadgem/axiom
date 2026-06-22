#pragma once
#include "Prim.hpp"
#include "slang-rhi.h"

namespace axm {
    class Utils {
    public:
        static rhi::ITexture* CreateDepthTexture(rhi::IDevice* device, u32 w, u32 h, rhi::Format format = rhi::Format::D32Float);

    };
}