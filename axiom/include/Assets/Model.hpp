#pragma once

#include "Assets/AssetManager.hpp"
#include "Core/STL.hpp"
#include "Render/Mesh.hpp"
#include "Render/Texture.hpp"
#include "assimp/Importer.hpp"

struct aiScene;

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

            DynArray<Map> m_TextureMaps;
        };

        struct MeshEntry
        {
            Mesh m_Mesh;
            u32  m_MaterialIndex;
        };

        DynArray<MeshEntry>     m_Meshes;
        DynArray<MaterialEntry> m_Materials;
    };

    struct ModelTransientData
    {
        Assimp::Importer m_Importer;
        const aiScene*   m_Scene;
    };

    using ModelAsset          = AssetT<Model, AssetType::Model>;
    using ModelAssetTransient = AssetTransientT<Model, ModelTransientData, AssetType::Model>;

    class ModelAssetFactory : public AssetFactory
    {
    public:
        GPU& m_GPU;

        explicit ModelAssetFactory(GPU& gpu);

        NO_DISCARD AssetLoadResult LoadAsset(const Filesystem::path& path) const override;
        void                       UnloadAsset(Asset* asset) const override;
        void                       ProcessAssetTransient(AssetTransient* data) const override;
        void                       OnTransientComplete(AssetTransient* data) const override;

        static DynArray<AssetLoadInfo>
        ProcessSceneMaterials(const String& directory, const aiScene* scene, ModelAsset* model);
    };

} // namespace axm
