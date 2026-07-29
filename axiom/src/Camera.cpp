#include "Render/Camera.hpp"
#include "Core/Utils.hpp"

JPH::Mat44 axm::Camera::GetViewProjectionMatrix() const {
    static const aml::Vec3 IDENTITY_SCALE = { 1.0f, 1.0f, 1.0f };
    const auto             view           = Utils::CreateViewMatrix(m_Transform.m_Position, m_Transform.m_Euler);
    const auto             aspect         = m_ViewportDimensions.x / m_ViewportDimensions.y;
    // TODO: Add support for other projection types
    const auto proj = aml::Mat44::sPerspective(aml::DegreesToRadians(m_FOV), aspect, m_NearPlane, m_FarPlane);

    return proj * view;
}
axm::FlyCamController::FlyCamController(const Input& input) : m_Input(input) { }

void axm::FlyCamController::Update(Camera& cam, f32 deltaTime) const {

    if (!m_Input.IsMouseButtonDown(MouseButton::Right)) {
        return;
    }

    const auto mouseVelocity = m_Input.GetMouseVelocity(cam.m_ViewportDimensions);

    f32        pitch         = cam.m_Transform.m_Euler.GetX() - (mouseVelocity.y * m_RotationSpeed);
    f32        yaw           = cam.m_Transform.m_Euler.GetY() - (mouseVelocity.x * m_RotationSpeed);

    pitch                    = aml::Clamp(pitch, -89.0f, 89.0f);

    cam.m_Transform.m_Euler.SetX(pitch);
    cam.m_Transform.m_Euler.SetY(yaw);
    cam.m_Transform.UpdateDirectionVectors();

    aml::Vec3 velocity = { 0.0f, 0.0f, 0.0f };

    if (m_Input.IsKeyDown(Keycode::W)) {
        velocity.SetZ(1.0f);
    } else if (m_Input.IsKeyDown(Keycode::S)) {
        velocity.SetZ(-1.0f);
    }
    if (m_Input.IsKeyDown(Keycode::D)) {
        velocity.SetX(1.0f);
    } else if (m_Input.IsKeyDown(Keycode::A)) {
        velocity.SetX(-1.0f);
    }
    if (m_Input.IsKeyDown(Keycode::E)) {
        velocity.SetY(1.0f);
    } else if (m_Input.IsKeyDown(Keycode::Q)) {
        velocity.SetY(-1.0f);
    }

    const f32 speedFactor = m_MovementSpeed * deltaTime;

    if (velocity.GetZ() != 0.0f) {
        cam.m_Transform.m_Position += cam.m_Transform.m_Forward * (velocity.GetZ() * speedFactor);
    }
    if (velocity.GetX() != 0.0f) {
        cam.m_Transform.m_Position += cam.m_Transform.m_Right * (velocity.GetX() * speedFactor);
    }
    if (velocity.GetY() != 0.0f) {
        cam.m_Transform.m_Position += cam.m_Transform.m_Up * (velocity.GetY() * speedFactor);
    }
}
