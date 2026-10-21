#pragma once

#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Alloc.hpp"
#include "Core/Debug.hpp"
#include "Core/Decorators.hpp"
#include "Core/Engine.hpp"
#include "Core/Input.hpp"
#include "Core/JSON.hpp"
#include "Core/Maths.hpp"
#include "Core/Prim.hpp"
#include "Core/Profile.hpp"
#include "Core/STL.hpp"
#include "Core/Timer.hpp"
#include "Core/Utils.hpp"
#include "Render/Camera.hpp"
#include "Render/ImGuiUtils.hpp"
#include "Render/Mesh.hpp"
#include "Render/Pipeline.hpp"
#include "Render/Shader.hpp"
#include "Render/Shapes.hpp"
#include "Render/Texture.hpp"
#include "Render/Transform.hpp"
#include "Render/Vertex.hpp"
#include "Render/Viewport.hpp"

// VKU
#include "ImGui/vku_extensions.h"

// Third party includes
// TODO: (Do we want these in the public header?)
#include "ImGuizmo.h"
#include "assimp/Importer.hpp"
#include "flecs.h"
#include "im3d.h"
#include "imgui.h"
#include "nanovg.h"
