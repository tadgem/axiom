#include "Assets/Model.hpp"

#include "Core/Profile.hpp"
#include "assimp/Importer.hpp"
#include "assimp/cimport.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

static axm::TextureMapType GetTextureTypeFromAssimp(const aiTextureType& t) {
    switch (t) {
        case aiTextureType_DIFFUSE:
        case aiTextureType_BASE_COLOR:
            return axm::TextureMapType::Diffuse;
        case aiTextureType_NORMALS:
            return axm::TextureMapType::Normal;
        case aiTextureType_AMBIENT_OCCLUSION:
            return axm::TextureMapType::AO;
        case aiTextureType_SPECULAR:
            return axm::TextureMapType::Specular;
        case aiTextureType_METALNESS:
            return axm::TextureMapType::Metallic;
        case aiTextureType_HEIGHT:
            return axm::TextureMapType::Height;
        case aiTextureType_DISPLACEMENT:
            return axm::TextureMapType::Displacement;
        case aiTextureType_OPACITY:
            return axm::TextureMapType::Opacity;
        case aiTextureType_DIFFUSE_ROUGHNESS:
            return axm::TextureMapType::Roughness;
        case aiTextureType_EMISSIVE:
            return axm::TextureMapType::Emissive;
        default:
            return axm::TextureMapType::Unknown;
    }
}


static aiTextureType GetAssimpTextureType(const axm::TextureMapType& t) {
    switch (t) {
        case axm::TextureMapType::Diffuse:
            return aiTextureType_DIFFUSE;
        case axm::TextureMapType::Normal:
            return aiTextureType_NORMALS;
        case axm::TextureMapType::AO:
            return aiTextureType_AMBIENT_OCCLUSION;
        case axm::TextureMapType::Specular:
            return aiTextureType_SPECULAR;
        case axm::TextureMapType::Metallic:
            return aiTextureType_METALNESS;
        case axm::TextureMapType::Height:
            return aiTextureType_HEIGHT;
        case axm::TextureMapType::Displacement:
            return aiTextureType_DISPLACEMENT;
        case axm::TextureMapType::Opacity:
            return aiTextureType_OPACITY;
        case axm::TextureMapType::Roughness:
            return aiTextureType_DIFFUSE_ROUGHNESS;
        case axm::TextureMapType::Emissive:
            return aiTextureType_EMISSIVE;
        default:
            return aiTextureType_UNKNOWN;
    }
}


axm::ModelAssetFactory::ModelAssetFactory(rhi::IDevice* gpuDevice) :
    AssetFactory(AssetType::Model), m_Device(gpuDevice) { }

axm::AssetLoadResult axm::ModelAssetFactory::LoadAsset(const String& path) const {
    Assimp::Importer      importer;

    static constexpr auto flags = aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes
                                  | aiProcess_GenSmoothNormals | aiProcess_OptimizeGraph | aiProcess_FixInfacingNormals
                                  | aiProcess_FindInvalidData | aiProcess_GenBoundingBoxes;


    const auto*     scene  = importer.ReadFile(path.c_str(), flags);

    AssetLoadResult result = { };

    if (!scene) {
        result.m_Next = AssetErrorMessage { .m_Message = "ModelAssetFactory : failed to open file at " + path };
        return result;
    }

    auto* modelAsset = AXM_NEW(ModelAsset, path, { });

    auto nextAssets = ProcessSceneMaterials("replace string in assets with a std::filesystem::path", scene, modelAsset);
    
    auto* transient = AXM_NEW(ModelAssetTransient, modelAsset, std::move(scene));

    transient->m_NumSteps = scene->mNumMeshes;
    result.m_Next         = dynamic_cast<AssetTransient*>(transient);
    return result;
}
void axm::ModelAssetFactory::UnloadAsset(Asset* asset) const { }
void axm::ModelAssetFactory::ProcessAssetTransient(AssetTransient* data) const {
    AssetFactory::ProcessAssetTransient(data);
}

axm::Model::MaterialEntry::Map GetMaterialTexture(const axm::String&  directory,
                                                  aiMaterial*         material,
                                                  aiTextureType       assimpTextureType,
                                                  axm::TextureMapType textureType) {
    PROFILE_SCOPE()
    using namespace axm;
    uint32_t tex_count = aiGetMaterialTextureCount(material, assimpTextureType);
    if (tex_count > 0) {
        aiString resultPath;
        aiGetMaterialTexture(material, assimpTextureType, 0, &resultPath);
        const String      final_path = directory + String(resultPath.C_Str());

        const AssetHandle h(final_path, AssetType::Texture);

        return { .m_MapType = textureType, .m_Handle = h };
    }

    return { .m_MapType = TextureMapType::Unknown, .m_Handle = AssetHandle::BAD };
}

axm::Vector<axm::AssetHandle>
axm::ModelAssetFactory::ProcessSceneMaterials(const String& directory, const aiScene* scene, ModelAsset* model) {
    // get all materials here
    PROFILE_SCOPE();
    Vector<AssetHandle> texturesToLoad = { };

    for (auto i = 0; i < scene->mNumMaterials; i++) {
        auto*                material = scene->mMaterials[i];

        Model::MaterialEntry entry    = { };

        for (auto j = 0; j < TextureMapType::Count; j++) {
            auto mapType = static_cast<TextureMapType>(j);
            auto map     = GetMaterialTexture(directory, material, GetAssimpTextureType(mapType), mapType);
            if (map.m_Handle != AssetHandle::BAD) {
                entry.m_TextureMaps.push_back(std::move(map));
            }
            if (std::ranges::find(texturesToLoad.begin(), texturesToLoad.end(), map.m_Handle) == texturesToLoad.end()) {
                texturesToLoad.push_back(map.m_Handle);
            }
        }

        model->m_Data.m_Materials.push_back(std::move(entry));
    }

    return texturesToLoad;

    // get all new textures to load
}
