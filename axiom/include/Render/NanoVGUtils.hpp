#pragma once

#include "Render/GPU.hpp"
#include "nanovg_impl_slang_rhi.h"
#include <nanovg.h>

namespace axm::nanovg {

    // Context creation & destruction
    NVGcontext* CreateContext(rhi::IDevice* device, int flags = NVG_ANTIALIAS);
    void        DestroyContext(NVGcontext* ctx);

    // Create render pipeline for target color and depth formats
    bool CreatePipeline(NVGcontext* ctx, rhi::Format colorFormat, rhi::Format depthFormat = rhi::Format::Undefined);

    // Upload pending texture updates (must be called outside a render pass encoder)
    void UpdateTextures(NVGcontext* ctx, rhi::ICommandEncoder* commandEncoder);

    // Render accumulated NVGcontext* draw commands to an active render pass encoder
    void Render(NVGcontext*             ctx,
                rhi::ICommandEncoder*    commandEncoder,
                rhi::IRenderPassEncoder* renderPassEncoder,
                float                    width,
                float                    height);

    // Register an existing RHI texture view as a NanoVG image handle
    int  CreateImageFromTextureView(NVGcontext* ctx, rhi::ITextureView* textureView);
    void DeleteImage(NVGcontext* ctx, int image);

} // namespace axm::nanovg
