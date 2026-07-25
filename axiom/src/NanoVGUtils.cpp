#include "Render/NanoVGUtils.hpp"
#include "nanovg_impl_slang_rhi.h"

namespace axm::nanovg {

    NVGcontext* CreateContext(rhi::IDevice* device, int flags) {
        return nvgCreateSlangRHI(device, flags);
    }

    void DestroyContext(NVGcontext* ctx) {
        nvgDeleteSlangRHI(ctx);
    }

    bool CreatePipeline(NVGcontext* ctx, rhi::Format colorFormat, rhi::Format depthFormat) {
        return nvgSlangRHICreatePipeline(ctx, colorFormat, depthFormat);
    }

    void UpdateTextures(NVGcontext* ctx, rhi::ICommandEncoder* commandEncoder) {
        nvgSlangRHIUpdateTextures(ctx, commandEncoder);
    }

    void Render(NVGcontext*             ctx,
                rhi::ICommandEncoder*    commandEncoder,
                rhi::IRenderPassEncoder* renderPassEncoder,
                float                    width,
                float                    height) {
        nvgSlangRHIRender(ctx, commandEncoder, renderPassEncoder, width, height);
    }

    int CreateImageFromTextureView(NVGcontext* ctx, rhi::ITextureView* textureView) {
        return nvgSlangRHICreateImage(ctx, textureView);
    }

    void DeleteImage(NVGcontext* ctx, int image) {
        nvgSlangRHIDeleteImage(ctx, image);
    }

} // namespace axm::nanovg
