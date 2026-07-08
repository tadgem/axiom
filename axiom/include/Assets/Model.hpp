#pragma once

#include "Core/STL.hpp"
#include "Render/Mesh.hpp"
#include "Render/Texture.hpp"
namespace axm {


    class Model
    {
    public:
        Vector<Mesh> m_Meshes;
        Vector<Texture> m_Textures;
    };
} // namespace axm
