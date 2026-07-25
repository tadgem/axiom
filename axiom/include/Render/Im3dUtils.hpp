#pragma once

#include "Render/Camera.hpp"
#include "Render/Transform.hpp"
#include "Render/Viewport.hpp"
#include <im3d.h>
#include <slang-rhi.h>

namespace axm::im3d {

    void NewFrame(const Camera& cam, f32 deltaTime, aml::Float2 viewportSize);

    void SetLineWidthScale(float scale);
    float GetLineWidthScale();

    void PushSize();
    void PushSize(float size);
    void PopSize();
    void SetSize(float size);
    float GetSize();

    void PushColor();
    void PushColor(Im3d::Color color);
    void PopColor();
    void SetColor(Im3d::Color color);
    Im3d::Color GetColor();

    void PushDrawState();
    void PopDrawState();

    void Render(rhi::ICommandEncoder*    commandEncoder,
                rhi::IRenderPassEncoder* renderPassEncoder,
                aml::Float2              viewportSize,
                const Camera*            camera         = nullptr,
                rhi::Format              colorFormat    = rhi::Format::Undefined,
                rhi::Format              depthFormat    = rhi::Format::Undefined,
                float                    lineWidthScale = 0.0f);

    bool TransformGizmo(const char* id, Transform& transform);

} // namespace axm::im3d
