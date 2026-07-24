#include "Render/Transform.hpp"
#include "Core/Utils.hpp"

JPH::Mat44 axm::Transform::GetModelMatrix() const { return Utils::CreateModelMatrix(m_Position, m_Euler, m_Scale); }
