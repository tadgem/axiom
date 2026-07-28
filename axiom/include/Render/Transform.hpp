#pragma once
#include "Core/Maths.hpp"

namespace axm {
    struct Transform
    {
        Transform()           = default;

        aml::Vec3  m_Position = aml::Vec3(0.0f, 0.0f, 0.0f);
        aml::Vec3  m_Euler    = aml::Vec3(0.0f, 0.0f, 0.0f);
        aml::Vec3  m_Scale    = aml::Vec3(1.0f, 1.0f, 1.0f);

        aml::Vec3  m_Forward  = aml::Vec3(0.0f, 0.0f, 1.0f);
        aml::Vec3  m_Right    = aml::Vec3(1.0f, 0.0f, 0.0f);
        aml::Vec3  m_Up       = aml::Vec3(0.0f, 1.0f, 0.0f);

        void       UpdateDirectionVectors();

        NO_DISCARD aml::Mat44 GetModelMatrix() const;
    };
}
