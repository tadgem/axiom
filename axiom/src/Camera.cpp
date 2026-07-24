#include "Render/Camera.hpp"

#include "Core/Utils.hpp"
JPH::Mat44 axm::Camera::GetViewProjectionMatrix() const {
    static const aml::Vec3 IDENTITY_SCALE = { 1.0f, 1.0f, 1.0f };
    const auto             view           = m_Transform.GetModelMatrix();
    const auto             aspect         = m_ViewportDimensions.x / m_ViewportDimensions.y;
    // TODO: Add support for other projection types
    const auto proj = aml::Mat44::sPerspective(aml::DegreesToRadians(m_FOV), aspect, m_NearPlane, m_FarPlane);

    return proj * view;
}
