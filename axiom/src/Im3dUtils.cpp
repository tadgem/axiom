#include "Render/Im3dUtils.hpp"
#include "im3d_impl_slang_rhi.h"

namespace axm::im3d {

    void NewFrame(const Camera& cam, f32 deltaTime, aml::Float2 viewportSize) {
        const auto viewMat = cam.m_Transform.GetModelMatrix();
        const auto aspect  = viewportSize.x / viewportSize.y;
        const auto projMat = aml::Mat44::sPerspective(aml::DegreesToRadians(cam.m_FOV), aspect, cam.m_NearPlane, cam.m_FarPlane);

        float view16[16];
        float proj16[16];
        viewMat.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(view16));
        projMat.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(proj16));

        const float camPos[3] = { cam.m_Transform.m_Position.GetX(), cam.m_Transform.m_Position.GetY(), cam.m_Transform.m_Position.GetZ() };
        const float camDir[3] = { 0.0f, 0.0f, -1.0f };

        Im3d_ImplSlangRHI_NewFrame(deltaTime,
                                   viewportSize.x,
                                   viewportSize.y,
                                   view16,
                                   proj16,
                                   camPos,
                                   camDir,
                                   aml::DegreesToRadians(cam.m_FOV));
    }

    void SetLineWidthScale(float scale) {
        Im3d_ImplSlangRHI_SetLineWidthScale(scale);
    }

    float GetLineWidthScale() {
        return Im3d_ImplSlangRHI_GetLineWidthScale();
    }

    void PushSize() {
        Im3d::PushSize();
    }

    void PushSize(float size) {
        Im3d::PushSize(size);
    }

    void PopSize() {
        Im3d::PopSize();
    }

    void SetSize(float size) {
        Im3d::SetSize(size);
    }

    float GetSize() {
        return Im3d::GetSize();
    }

    void PushColor() {
        Im3d::PushColor();
    }

    void PushColor(Im3d::Color color) {
        Im3d::PushColor(color);
    }

    void PopColor() {
        Im3d::PopColor();
    }

    void SetColor(Im3d::Color color) {
        Im3d::SetColor(color);
    }

    Im3d::Color GetColor() {
        return Im3d::GetColor();
    }

    void PushDrawState() {
        Im3d::PushDrawState();
    }

    void PopDrawState() {
        Im3d::PopDrawState();
    }

    void Render(rhi::ICommandEncoder*    commandEncoder,
                rhi::IRenderPassEncoder* renderPassEncoder,
                aml::Float2              viewportSize,
                const Camera*            camera,
                rhi::Format              colorFormat,
                rhi::Format              depthFormat,
                float                    lineWidthScale) {
        float vp16[16];
        const float* vpPtr = nullptr;

        if (camera) {
            auto vp = camera->GetViewProjectionMatrix();
            vp.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(vp16));
            vpPtr = vp16;
        }

        Im3d_ImplSlangRHI_RenderDrawData(commandEncoder,
                                         renderPassEncoder,
                                         viewportSize.x,
                                         viewportSize.y,
                                         vpPtr,
                                         colorFormat,
                                         depthFormat,
                                         lineWidthScale);
    }

    bool TransformGizmo(const char* id, Transform& transform) {
        aml::Mat44 modelMat = transform.GetModelMatrix();
        float mat16[16];
        modelMat.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(mat16));

        if (Im3d::Gizmo(id, mat16)) {
            transform.m_Position = aml::Vec3(mat16[12], mat16[13], mat16[14]);
            return true;
        }

        return false;
    }

} // namespace axm::im3d
