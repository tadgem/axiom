#pragma once
#include "Render/Camera.hpp"
#include "Render/Transform.hpp"

namespace axm::ImGuiEx {
    bool TransformEdit(Transform& trans);

    bool CameraEdit(Camera& cam);

    bool FlyCamControllerEdit(FlyCamController& controller);

}
