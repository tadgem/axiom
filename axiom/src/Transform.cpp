#include "Render/Transform.hpp"
#include "Core/Utils.hpp"

void axm::Transform::UpdateDirectionVectors() {
    const auto eulerRadians = aml::Vec3 { aml::DegreesToRadians(m_Euler.GetX()),
                                          aml::DegreesToRadians(m_Euler.GetY()),
                                          aml::DegreesToRadians(m_Euler.GetZ()) };

    m_Forward.mF32[0]       = aml::Cos(eulerRadians.GetX()) * aml::Sin(eulerRadians.GetY());
    m_Forward.mF32[1]       = -aml::Sin(eulerRadians.GetX());
    m_Forward.mF32[2]       = aml::Cos(eulerRadians.GetX()) * aml::Cos(eulerRadians.GetY());
    m_Forward.mF32[3]       = m_Forward.mF32[2];

    m_Right.mF32[0]         = aml::Cos(eulerRadians.GetY());
    m_Right.mF32[1]         = 0.0f;
    m_Right.mF32[2]         = -aml::Sin(eulerRadians.GetY());
    m_Right.mF32[3]         = m_Right.mF32[2];


    m_Up.mF32[0]            = aml::Sin(eulerRadians.GetX()) * aml::Sin(eulerRadians.GetY());
    m_Up.mF32[1]            = aml::Cos(eulerRadians.GetX());
    m_Up.mF32[2]            = aml::Sin(eulerRadians.GetX()) * aml::Cos(eulerRadians.GetX());
    m_Up.mF32[3]            = m_Up.mF32[2];
}
JPH::Mat44 axm::Transform::GetModelMatrix() const { return Utils::CreateModelMatrix(m_Position, m_Euler, m_Scale); }
