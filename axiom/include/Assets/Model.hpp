#pragma once

#include "Assets/Assets.hpp"
#include "Core/STL.hpp"
#include "Render/Mesh.hpp"
#include "Render/Texture.hpp"
namespace axm {


    class Model
    {
    public:
        struct MaterialEntry
        {
            struct Map
            {
                TextureMapType m_MapType;
                AssetHandle    m_Handle;
            };

            Vector<Map> m_TextureMaps;
        };

        struct MeshEntry
        {
            Mesh m_Mesh;
            u16  m_MaterialIndex;
        };

        Vector<Mesh>          m_Meshes;
        Vector<MaterialEntry> m_TextureHandles;
    };
} // namespace axm
