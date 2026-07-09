#pragma once

#include "Assets/Assets.hpp"
#include "Core/STL.hpp"
#include "Render/Mesh.hpp"
#include "Render/Texture.hpp"
namespace axm {


    class Model
    {
    public:
        Vector<Mesh>                  m_Meshes;
        Vector<AssetHandle>           m_TextureHandles;
        HashMap<AssetHandle, Texture> m_Textures;
    };
} // namespace axm
