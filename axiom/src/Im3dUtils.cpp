#include "Render/Im3dUtils.hpp"
#include "im3d_impl_slang_rhi.h"

namespace axm::SlangIm3D {

    void NewFrame(const Camera& cam, f32 deltaTime, aml::Float2 viewportSize) {
        const auto viewMat = cam.m_Transform.GetModelMatrix();
        const auto aspect  = viewportSize.x / viewportSize.y;
        const auto projMat
                = aml::Mat44::sPerspective(aml::DegreesToRadians(cam.m_FOV), aspect, cam.m_NearPlane, cam.m_FarPlane);

        f32 view16[16];
        f32 proj16[16];
        viewMat.StoreFloat4x4(reinterpret_cast<aml::Float4*>(view16));
        projMat.StoreFloat4x4(reinterpret_cast<aml::Float4*>(proj16));

        const f32 camPos[3] = { cam.m_Transform.m_Position.GetX(),
                                cam.m_Transform.m_Position.GetY(),
                                cam.m_Transform.m_Position.GetZ() };
        const f32 camDir[3] = { 0.0f, 0.0f, -1.0f };

        Im3d_ImplSlangRHI_NewFrame(deltaTime,
                                   viewportSize.x,
                                   viewportSize.y,
                                   view16,
                                   proj16,
                                   camPos,
                                   camDir,
                                   aml::DegreesToRadians(cam.m_FOV));
    }

    void SetLineWidthScale(f32 scale) { Im3d_ImplSlangRHI_SetLineWidthScale(scale); }

    f32  GetLineWidthScale() { return Im3d_ImplSlangRHI_GetLineWidthScale(); }

    void Render(rhi::ICommandEncoder*    commandEncoder,
                rhi::IRenderPassEncoder* renderPassEncoder,
                aml::Float2              viewportSize,
                const Camera&            camera,
                rhi::Format              colorFormat,
                rhi::Format              depthFormat,
                f32                      lineWidthScale) {
        f32        vp16[16];
        const f32* vpPtr = nullptr;

        const auto vp    = camera.GetViewProjectionMatrix();
        vp.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(vp16));
        vpPtr = vp16;


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
        f32        mat16[16];
        modelMat.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(mat16));

        if (Im3d::Gizmo(id, mat16)) {
            transform.m_Position = aml::Vec3(mat16[12], mat16[13], mat16[14]);
            return true;
        }

        return false;
    }

} // namespace axm::im3d
