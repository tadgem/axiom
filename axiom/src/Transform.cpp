#include "Render/Transform.hpp"
#include "Core/Utils.hpp"

void axm::Transform::UpdateDirectionVectors() {
    const auto eulerRadians = aml::Vec3 { aml::DegreesToRadians(m_Euler.GetX()),
                                          aml::DegreesToRadians(m_Euler.GetY()),
                                          aml::DegreesToRadians(m_Euler.GetZ()) };
    const auto quat         = aml::Quat::sEulerAngles(eulerRadians);
    const auto rot          = aml::Mat44::sRotation(quat);

    m_Right   = rot.GetAxisX();
    m_Up      = rot.GetAxisY();
    m_Forward = -rot.GetAxisZ();
}
JPH::Mat44 axm::Transform::GetModelMatrix() const { return Utils::CreateModelMatrix(m_Position, m_Euler, m_Scale); }
