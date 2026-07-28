#include "Render/Transform.hpp"
#include "Core/Utils.hpp"

void axm::Transform::UpdateDirectionVectors() {
    const auto eulerRadians = aml::Vec3 { aml::DegreesToRadians(m_Euler.GetX()),
                                          aml::DegreesToRadians(m_Euler.GetY()),
                                          aml::DegreesToRadians(m_Euler.GetZ()) };
    aml::Quat  rotation     = aml::Quat::sEulerAngles(eulerRadians);
    m_Right                 = rotation.RotateAxisX();
    m_Up                    = rotation.RotateAxisY();
    m_Forward               = rotation.RotateAxisZ();
}
JPH::Mat44 axm::Transform::GetModelMatrix() const { return Utils::CreateModelMatrix(m_Position, m_Euler, m_Scale); }
