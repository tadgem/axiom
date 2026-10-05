#include "Assets/Model.hpp"
#include "Core/Profile.hpp"
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

axm::ModelAssetFactory::ModelAssetFactory(GPU& gpu) : AssetFactory(AssetType::Model), m_GPU(gpu) { }

axm::AssetLoadResult axm::ModelAssetFactory::LoadAsset(const Filesystem::path& path) const {
    PROFILE_SCOPE()

    static constexpr auto IMPORT_FLAGS = aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes
                                         | aiProcess_GenSmoothNormals | aiProcess_OptimizeGraph
                                         | aiProcess_FixInfacingNormals | aiProcess_FindInvalidData
                                         | aiProcess_GenBoundingBoxes | aiProcess_FlipWindingOrder;


    AssetLoadResult result     = { };


    auto*           modelAsset = AXM_NEW(ModelAsset, String(path.string()), { });
    auto*           transient  = AXM_NEW(ModelAssetTransient, modelAsset);
    transient->m_TransientData.m_Scene
            = transient->m_TransientData.m_Importer.ReadFile(path.string().c_str(), IMPORT_FLAGS);


    if (!transient->m_TransientData.m_Scene) {
        result.m_Next = AssetErrorMessage { .m_Message
                                            = "ModelAssetFactory : failed to open file at " + String(path.string()) };
        return result;
    }

    const auto dir         = String(path.parent_path().string()) + "/";
    auto       nextAssets  = ProcessSceneMaterials(dir, transient->m_TransientData.m_Scene, modelAsset);


    transient->m_NumSteps  = transient->m_TransientData.m_Scene->mNumMeshes;
    result.m_Next          = dynamic_cast<AssetTransient*>(transient);
    result.m_NewAssetTasks = std::move(nextAssets);
    return result;
}

void axm::ModelAssetFactory::UnloadAsset(Asset* asset) const {
    PROFILE_SCOPE()
    auto* model = dynamic_cast<ModelAsset*>(asset);
    if (model == nullptr) {
        return;
    }

    AXM_LOG_INFO("ModelAssetFactory::UnloadAsset freeing {} meshes", model->m_Data.m_Meshes.size());
    for (auto& entry: model->m_Data.m_Meshes) {
        entry.m_Mesh.Free(m_GPU.State());
    }
    model->m_Data.m_Meshes.clear();
}

thread_local axm::DynArray<f32> g_VertexDataCommitBuffer;
thread_local axm::DynArray<u32> g_IndexDataCommitBuffer;

void axm::ModelAssetFactory::ProcessAssetTransient(AssetTransient* data) const {
    PROFILE_SCOPE()
    auto*       transient = dynamic_cast<ModelAssetTransient*>(data);
    ModelAsset* model     = transient->GetConcreteAsset();
    auto*       mesh      = transient->m_TransientData.m_Scene->mMeshes[transient->m_CurrentStep++];

    if (!mesh->HasNormals()) {
        AXM_LOG_ERROR("Mesh {} does not have any normals", model->m_Path.generic_string());
    }

    g_VertexDataCommitBuffer.clear();
    for (auto i = 0; i < mesh->mNumVertices; i++) {
        // positions
        g_VertexDataCommitBuffer.push_back(mesh->mVertices[i].x);
        g_VertexDataCommitBuffer.push_back(mesh->mVertices[i].y);
        g_VertexDataCommitBuffer.push_back(mesh->mVertices[i].z);

        // normals
        if (mesh->HasNormals()) {
            g_VertexDataCommitBuffer.push_back(mesh->mNormals[i].x);
            g_VertexDataCommitBuffer.push_back(mesh->mNormals[i].y);
            g_VertexDataCommitBuffer.push_back(mesh->mNormals[i].z);
        }

        // todo: support additional tex coords
        if (mesh->HasTextureCoords(0)) {
            g_VertexDataCommitBuffer.push_back(mesh->mTextureCoords[0][i].x);
            g_VertexDataCommitBuffer.push_back(mesh->mTextureCoords[0][i].y);
        }
    }

    g_IndexDataCommitBuffer.clear();
    for (auto i = 0; i < mesh->mNumFaces; i++) {
        if (mesh->mFaces[i].mNumIndices != 3) {
            AXM_LOG_ERROR("Mesh {} has non triangular face.", model->m_Path.generic_string());
            continue;
        }
        for (unsigned int index = 0; index < mesh->mFaces[i].mNumIndices; index++) {
            g_IndexDataCommitBuffer.push_back(CAST(mesh->mFaces[i].mIndices[index], u32));
        }
    }

    auto gpuMesh = meshes::CreateMeshFromData(m_GPU.State(),
                                              g_VertexDataCommitBuffer.data(),
                                              g_VertexDataCommitBuffer.size() * sizeof(f32),
                                              g_IndexDataCommitBuffer.data(),
                                              g_IndexDataCommitBuffer.size(),
                                              mesh->mName.C_Str());
    model->m_Data.m_Meshes.push_back({ .m_Mesh = std::move(gpuMesh), .m_MaterialIndex = mesh->mMaterialIndex });
}


void axm::ModelAssetFactory::OnTransientComplete(AssetTransient* data) const {
    PROFILE_SCOPE()
    auto*       transient = dynamic_cast<ModelAssetTransient*>(data);
    
    transient->m_TransientData.m_Importer.FreeScene();
}

static axm::Pair<axm::Model::MaterialEntry::Map, axm::String>
GetMaterialTexture(const axm::String&         directory,
                   const aiMaterial*          material,
                   const aiTextureType&       assimpTextureType,
                   const axm::TextureMapType& textureType) {
    PROFILE_SCOPE()
    using namespace axm;
    const auto textureCount = aiGetMaterialTextureCount(material, assimpTextureType);
    if (textureCount > 0) {
        aiString resultPath;
        aiGetMaterialTexture(material, assimpTextureType, 0, &resultPath);
        const String      finalPath = directory + String(resultPath.C_Str());

        const AssetHandle h(finalPath, AssetType::Texture);

        return { { .m_MapType = textureType, .m_Handle = h }, finalPath };
    }

    return { { .m_MapType = TextureMapType::Unknown, .m_Handle = AssetHandle::BAD }, "" };
}

axm::DynArray<axm::AssetLoadInfo>
axm::ModelAssetFactory::ProcessSceneMaterials(const String& directory, const aiScene* scene, ModelAsset* model) {
    // get all materials here
    PROFILE_SCOPE();
    DynArray<AssetLoadInfo> texturesToLoad = { };

    for (auto i = 0; i < scene->mNumMaterials; i++) {
        const auto*          material = scene->mMaterials[i];

        Model::MaterialEntry entry    = { };

        for (auto j = 0; j < TextureMapType::Count; j++) {
            auto mapType            = CAST(j, TextureMapType);
            const auto& [map, path] = GetMaterialTexture(directory, material, GetAssimpTextureType(mapType), mapType);

            if (map.m_Handle != AssetHandle::BAD) {
                entry.m_TextureMaps.push_back(std::move(map));
                const auto loadInfo = AssetLoadInfo { .m_Path = path, .m_AssetType = AssetType::Texture };
                if (std::ranges::find(texturesToLoad.begin(), texturesToLoad.end(), loadInfo) == texturesToLoad.end()) {
                    texturesToLoad.push_back(loadInfo);
                }
            }
        }
        model->m_Data.m_Materials.push_back(std::move(entry));
    }

    return texturesToLoad;
}
