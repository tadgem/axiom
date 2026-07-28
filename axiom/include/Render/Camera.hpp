#pragma once
#include "Core/Input.hpp"
#include "Core/Maths.hpp"
#include "Render/Transform.hpp"

namespace axm {
    struct Camera
    {
        Camera() = default;

        enum class ProjectionType { Perspective, Unknown };

        Transform      m_Transform;
        aml::Float2    m_ViewportDimensions = aml::Float2(16.0f, 9.0f);
        f32            m_FOV = 60.0f, m_NearPlane = 0.03f, m_FarPlane = 1000.0f;
        ProjectionType m_ProjectionType = ProjectionType::Perspective;

        NO_DISCARD aml::Mat44 GetViewProjectionMatrix() const;
    };

    class FlyCamController
    {
    public:
        const Input& m_Input;

        f32          m_MovementSpeed = 3.0f;
        f32          m_RotationSpeed = 60.0f;

        FlyCamController(const Input& input);
        void Update(Camera& cam);
    };
}
