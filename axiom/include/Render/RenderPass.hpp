#pragma once
#include "Core/Engine.hpp"
#include "Core/Maths.hpp"
#include "Core/STL.hpp"

namespace axm {
    namespace render_pass {
        static constexpr u8      kMaxColourTargets = 8;

        rhi::IRenderPassEncoder* BeginSwapChainRenderPass(GPU&                  gpu,
                                                          rhi::ICommandEncoder* cmd,
                                                          rhi::LoadOp           colourLoadOp    = rhi::LoadOp::Clear,
                                                          rhi::LoadOp           depthLoadOp     = rhi::LoadOp::Clear,
                                                          bool                  enableDepthTest = true,
                                                          const aml::Vec4& clearColour = { 0.0f, 0.0f, 0.0f, 1.0f });

        class Builder
        {
        public:
            Array<rhi::RenderPassColorAttachment, kMaxColourTargets> m_ColourTargets;
            rhi::RenderPassDepthStencilAttachment                    m_DepthStencil;
            u32                                                      m_NumColourTargets = 0;
            bool                                                     m_HasDepthStencil  = false;

            Builder(rhi::ICommandEncoder* cmd);

            Builder&   AddColourTarget(rhi::ITextureView* texture, rhi::LoadOp loadOp, const aml::Vec4& clearColour);
            Builder&   AddDepthTarget(rhi::ITextureView* texture, rhi::LoadOp loadOp);
            Builder&   AddStencilTarget(rhi::LoadOp loadOp, rhi::StoreOp storeOp, u8 clearValue);
            NO_DISCARD rhi::IRenderPassEncoder* Build() const;

        private:
            rhi::ICommandEncoder* m_CommandEncoder;
        };
    };
} // namespace axm
