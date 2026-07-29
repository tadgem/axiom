#include "Render/ImGuiUtils.hpp"
#include "imgui.h"

bool axm::ImGuiEx::TransformEdit(Transform& trans) {
    bool b = false;
    if (ImGui::TreeNode("Transform")) {
        b |= ImGui::DragFloat3("Position", &trans.m_Position.mF32[0]);
        b |= ImGui::DragFloat3("Euler", &trans.m_Euler.mF32[0]);
        b |= ImGui::DragFloat3("Scale", &trans.m_Scale.mF32[0]);
        ImGui::TreePop();
    }

    return b;
}
bool axm::ImGuiEx::CameraEdit(Camera& cam) {
    bool b = false;
    if (ImGui::TreeNode("Camera")) {
        b |= TransformEdit(cam.m_Transform);
        b |= ImGui::SliderFloat("FOV", &cam.m_FOV, 1.0f, 120.0f);
        b |= ImGui::DragFloat("Near Clipping Plane", &cam.m_NearPlane);
        b |= ImGui::DragFloat("Far Clipping Plane", &cam.m_FarPlane);

        ImGui::TreePop();
    }

    return b;
}
bool axm::ImGuiEx::FlyCamControllerEdit(FlyCamController& controller) {
    bool b = false;
    if (ImGui::TreeNode("FlyCam Controller")) {
        b |= ImGui::DragFloat("Movement Speed", &controller.m_MovementSpeed);
        b |= ImGui::DragFloat("Rotation Speed", &controller.m_RotationSpeed);

        ImGui::TreePop();
    }

    return b;
}
