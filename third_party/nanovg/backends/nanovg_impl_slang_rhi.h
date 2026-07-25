#pragma once

#include <nanovg.h>
#include <slang-rhi.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NVGcreateFlags {
    NVG_ANTIALIAS       = 1 << 0,
    NVG_STENCIL_STROKES = 1 << 1,
    NVG_DEBUG           = 1 << 2,
};

// Create and destroy NanoVG Slang-RHI context
NVGcontext* nvgCreateSlangRHI(rhi::IDevice* device, int flags);
void        nvgDeleteSlangRHI(NVGcontext* ctx);

// Pre-create/compile pipeline for specific target color and depth formats
bool nvgSlangRHICreatePipeline(NVGcontext* ctx,
                               rhi::Format colorFormat,
                               rhi::Format depthFormat = rhi::Format::Undefined);

// Upload pending texture updates (must be called outside a render pass)
void nvgSlangRHIUpdateTextures(NVGcontext* ctx, rhi::ICommandEncoder* commandEncoder);

// Render accumulated NanoVG draw data into an active render pass encoder
void nvgSlangRHIRender(NVGcontext*             ctx,
                       rhi::ICommandEncoder*    commandEncoder,
                       rhi::IRenderPassEncoder* renderPassEncoder,
                       float                    width,
                       float                    height);

// Register an existing RHI texture view as a NanoVG image handle
int  nvgSlangRHICreateImage(NVGcontext* ctx, rhi::ITextureView* textureView);
void nvgSlangRHIDeleteImage(NVGcontext* ctx, int image);

#ifdef __cplusplus
}
#endif
