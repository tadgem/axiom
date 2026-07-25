#pragma once

#include <im3d.h>
#include <slang-rhi.h>
#include "Render/Camera.hpp"
#include "Render/Transform.hpp"
#include "Render/Viewport.hpp"

namespace axm::SlangIm3D {

    void NewFrame(const Camera& cam, f32 deltaTime, aml::Float2 viewportSize);

    void SetLineWidthScale(f32 scale);
    f32  GetLineWidthScale();

    void PushDrawState();
    void PopDrawState();

    void Render(rhi::ICommandEncoder*    commandEncoder,
                rhi::IRenderPassEncoder* renderPassEncoder,
                aml::Float2              viewportSize,
                const Camera&            camera,
                rhi::Format              colorFormat    = rhi::Format::Undefined,
                rhi::Format              depthFormat    = rhi::Format::Undefined,
                f32                      lineWidthScale = 0.0f);

    bool TransformGizmo(const char* id, Transform& transform);

} // namespace axm::im3d
