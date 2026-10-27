#include "Assets/Model.hpp"

#include "Assets/VfsppIO.hpp"
#include "Core/Profile.hpp"
#include "assimp/cimport.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

static axm::TextureMapType GetTextureTypeFromAssimp(const aiTextureType& t)
{
	switch (t)
	{
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

static aiTextureType GetAssimpTextureType(const axm::TextureMapType& t)
{
	switch (t)
	{
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

axm::ModelAssetFactory::ModelAssetFactory(vfspp::VirtualFileSystem& vfs, vku::VkState& vk)
    : AssetFactory(vfs, AssetType::Model), m_VK(vk)
{}

axm::AssetLoadResult axm::ModelAssetFactory::LoadAsset(const Filesystem::path& path) const
{
	PROFILE_SCOPE()

	static constexpr auto IMPORT_FLAGS =
	        aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes | aiProcess_GenSmoothNormals |
	        aiProcess_OptimizeGraph | aiProcess_LimitBoneWeights | aiProcess_FixInfacingNormals |
	        aiProcess_FindInvalidData | aiProcess_GenBoundingBoxes | aiProcess_FlipWindingOrder;

	AssetLoadResult result     = {};

	auto*           modelAsset = AXM_NEW(ModelAsset, String(path.string()), {});
	auto*           transient  = AXM_NEW(ModelAssetTransient, modelAsset);

	transient->m_TransientData.m_Importer.SetIOHandler(new VfsppIOSystem(m_FileSystem));
	transient->m_TransientData.m_Scene =
	        transient->m_TransientData.m_Importer.ReadFile(path.generic_string(), IMPORT_FLAGS);

	if (!transient->m_TransientData.m_Scene)
	{
		result.m_Next =
		        AssetErrorMessage{.m_Message = "ModelAssetFactory : failed to open file at " + String(path.string())};
		AXM_DELETE(ModelAssetTransient, transient);
		AXM_DELETE(ModelAsset, modelAsset);
		return result;
	}

	const auto dir         = String(path.parent_path().generic_string()) + "/";
	auto       nextAssets  = ProcessSceneMaterials(dir, transient->m_TransientData.m_Scene, modelAsset);

	transient->m_NumSteps  = transient->m_TransientData.m_Scene->mNumMeshes;
	result.m_Next          = dynamic_cast<AssetTransient*>(transient);
	result.m_NewAssetTasks = std::move(nextAssets);
	return result;
}

void axm::ModelAssetFactory::UnloadAsset(Asset* asset) const
{
	PROFILE_SCOPE()
	auto* model = dynamic_cast<ModelAsset*>(asset);
	if (model == nullptr)
	{
		return;
	}

	AXM_LOG_INFO("ModelAssetFactory::UnloadAsset freeing {} meshes", model->m_Data.m_Meshes.size());
	for (auto& entry : model->m_Data.m_Meshes)
	{
		entry.m_Mesh.Free(m_VK);
	}
	model->m_Data.m_Meshes.clear();

	for (auto& entry : model->m_Data.m_SkeletalMeshes)
	{
		entry.m_Mesh.Free(m_VK);
	}
	model->m_Data.m_SkeletalMeshes.clear();
}

thread_local axm::DynArray<u8>  g_VertexDataCommitBuffer;
thread_local axm::DynArray<u32> g_IndexDataCommitBuffer;

void                            PushFloatToVertexBuffer(float f)
{
	u8* begin = (g_VertexDataCommitBuffer.begin() + g_VertexDataCommitBuffer.size())._Ptr;

	g_VertexDataCommitBuffer.insert(g_VertexDataCommitBuffer.end(), {0, 0, 0, 0});

	memcpy(begin, &f, sizeof(float));
}

void PushBonesToVertexBuffer(axm::GPUMesh::VertexBones b)
{
	u8* begin = (g_VertexDataCommitBuffer.begin() + g_VertexDataCommitBuffer.size())._Ptr;

	for (auto i = 0; i < sizeof(b); i++)
	{
		g_VertexDataCommitBuffer.push_back(0);
	}

	memcpy(begin, &b, sizeof(float));
}

void ProcessSkeletalMesh(vku::VkState& vk, aiMesh* mesh, axm::ModelAsset* model)
{
	using namespace axm;
	g_VertexDataCommitBuffer.clear();
	// TODO(LiamD) : This is horrible, we need to account for varios vertex layouts..
	g_VertexDataCommitBuffer.reserve(CAST(mesh->mNumVertices * 8 * 4, size_t));
	g_IndexDataCommitBuffer.clear();
	g_IndexDataCommitBuffer.reserve(CAST(mesh->mNumFaces * 3, size_t));

	for (auto i = 0; i < mesh->mNumVertices; i++)
	{
		// positions
		PushFloatToVertexBuffer(mesh->mVertices[i].x);
		PushFloatToVertexBuffer(mesh->mVertices[i].y);
		PushFloatToVertexBuffer(mesh->mVertices[i].z);

		// normals
		if (mesh->HasNormals())
		{
			PushFloatToVertexBuffer(mesh->mNormals[i].x);
			PushFloatToVertexBuffer(mesh->mNormals[i].y);
			PushFloatToVertexBuffer(mesh->mNormals[i].z);
		}
		if (mesh->HasBones())
		{
			auto* b = mesh->mBones[0];

			// vertex bone attribute is 4 x 32 bytes [16:boneIdx][16:vertexWeight]
			int   i = 10;
		}
		// todo: support additional tex coords
		if (mesh->HasTextureCoords(0))
		{
			PushFloatToVertexBuffer(mesh->mTextureCoords[0][i].x);
			PushFloatToVertexBuffer(mesh->mTextureCoords[0][i].y);
		}

		if (mesh->HasBones())
		{
			auto*                b           = mesh->mBones[0];
			GPUMesh::VertexBones vertexBones = {};

			auto                 count       = 0;
			for (auto b = 0; b < mesh->mNumBones; b++)
			{
				aiBone* bone = mesh->mBones[b];
				for (auto w = 0; w < bone->mNumWeights; w++)
				{
					if (bone->mWeights[w].mVertexId == i)
					{
						vertexBones.m_BoneIndex[count]  = static_cast<u16>(b);
						vertexBones.m_BoneWeight[count] = f16(bone->mWeights[w].mWeight);
					}
				}
			}

			PushBonesToVertexBuffer(vertexBones);
		}
	}

	for (auto i = 0; i < mesh->mNumFaces; i++)
	{
		if (mesh->mFaces[i].mNumIndices != 3)
		{
			AXM_LOG_ERROR("Mesh {} has non triangular face.", model->m_Path.generic_string());
			continue;
		}
		for (unsigned int index = 0; index < mesh->mFaces[i].mNumIndices; index++)
		{
			g_IndexDataCommitBuffer.push_back(CAST(mesh->mFaces[i].mIndices[index], u32));
		}
	}

	auto gpuMesh = meshes::CreateMeshFromData(vk,
	                                          g_VertexDataCommitBuffer.data(),
	                                          g_VertexDataCommitBuffer.size() * sizeof(u8),
	                                          g_IndexDataCommitBuffer.data(),
	                                          g_IndexDataCommitBuffer.size(),
	                                          mesh->mName.C_Str());
	model->m_Data.m_SkeletalMeshes.push_back({.m_Mesh = std::move(gpuMesh), .m_MaterialIndex = mesh->mMaterialIndex});
}

void ProcessStaticMesh(vku::VkState& vk, aiMesh* mesh, axm::ModelAsset* model)
{
	using namespace axm;
	g_VertexDataCommitBuffer.clear();
	// TODO(LiamD) : This is horrible, we need to account for varios vertex layouts..
	g_VertexDataCommitBuffer.reserve(CAST(mesh->mNumVertices * 12 * 4, size_t));
	g_IndexDataCommitBuffer.clear();
	g_IndexDataCommitBuffer.reserve(CAST(mesh->mNumFaces * 3, size_t));

	for (auto i = 0; i < mesh->mNumVertices; i++)
	{
		// positions
		PushFloatToVertexBuffer(mesh->mVertices[i].x);
		PushFloatToVertexBuffer(mesh->mVertices[i].y);
		PushFloatToVertexBuffer(mesh->mVertices[i].z);

		// normals
		if (mesh->HasNormals())
		{
			PushFloatToVertexBuffer(mesh->mNormals[i].x);
			PushFloatToVertexBuffer(mesh->mNormals[i].y);
			PushFloatToVertexBuffer(mesh->mNormals[i].z);
		}

		// todo: support additional tex coords
		if (mesh->HasTextureCoords(0))
		{
			PushFloatToVertexBuffer(mesh->mTextureCoords[0][i].x);
			PushFloatToVertexBuffer(mesh->mTextureCoords[0][i].y);
		}
	}

	for (auto i = 0; i < mesh->mNumFaces; i++)
	{
		if (mesh->mFaces[i].mNumIndices != 3)
		{
			AXM_LOG_ERROR("Mesh {} has non triangular face.", model->m_Path.generic_string());
			continue;
		}
		for (unsigned int index = 0; index < mesh->mFaces[i].mNumIndices; index++)
		{
			g_IndexDataCommitBuffer.push_back(CAST(mesh->mFaces[i].mIndices[index], u32));
		}
	}

	auto gpuMesh = meshes::CreateMeshFromData(vk,
	                                          g_VertexDataCommitBuffer.data(),
	                                          g_VertexDataCommitBuffer.size() * sizeof(u8),
	                                          g_IndexDataCommitBuffer.data(),
	                                          g_IndexDataCommitBuffer.size(),
	                                          mesh->mName.C_Str());
	model->m_Data.m_Meshes.push_back({.m_Mesh = std::move(gpuMesh), .m_MaterialIndex = mesh->mMaterialIndex});
}

void axm::ModelAssetFactory::ProcessAssetTransient(AssetTransient* data) const
{
	PROFILE_SCOPE()
	auto*       transient = dynamic_cast<ModelAssetTransient*>(data);
	ModelAsset* model     = transient->GetConcreteAsset();
	auto*       mesh      = transient->m_TransientData.m_Scene->mMeshes[transient->m_CurrentStep++];

	if (!mesh->HasNormals())
	{
		AXM_LOG_ERROR("Mesh {} does not have any normals", model->m_Path.generic_string());
	}
	if (mesh->HasBones())
	{
		ProcessSkeletalMesh(m_VK, mesh, model);
	}
	else
	{
		ProcessStaticMesh(m_VK, mesh, model);
	}
}

void axm::ModelAssetFactory::OnTransientComplete(AssetTransient* data) const
{
	PROFILE_SCOPE()
	auto* transient = dynamic_cast<ModelAssetTransient*>(data);
	transient->m_TransientData.m_Importer.FreeScene();

	g_VertexDataCommitBuffer.clear();
	g_VertexDataCommitBuffer.shrink_to_fit();

	g_IndexDataCommitBuffer.clear();
	g_IndexDataCommitBuffer.shrink_to_fit();
}

static axm::Pair<axm::Model::MaterialEntry::Map, axm::String> GetMaterialTexture(const axm::String&   directory,
                                                                                 const aiMaterial*    material,
                                                                                 const aiTextureType& assimpTextureType,
                                                                                 const axm::TextureMapType& textureType)
{
	PROFILE_SCOPE()
	using namespace axm;
	const auto textureCount = aiGetMaterialTextureCount(material, assimpTextureType);
	if (textureCount > 0)
	{
		aiString resultPath;
		aiGetMaterialTexture(material, assimpTextureType, 0, &resultPath);
		const String      finalPath = directory + String(resultPath.C_Str());

		const AssetHandle h(finalPath, AssetType::Texture);

		return {{.m_MapType = textureType, .m_Handle = h}, finalPath};
	}

	return {{.m_MapType = TextureMapType::Unknown, .m_Handle = AssetHandle::BAD}, ""};
}

axm::DynArray<axm::AssetLoadInfo> axm::ModelAssetFactory::ProcessSceneMaterials(const String&  directory,
                                                                                const aiScene* scene,
                                                                                ModelAsset*    model)
{
	// get all materials here
	PROFILE_SCOPE();
	DynArray<AssetLoadInfo> texturesToLoad = {};

	for (auto i = 0; i < scene->mNumMaterials; i++)
	{
		const auto*          material = scene->mMaterials[i];

		Model::MaterialEntry entry    = {};

		for (auto j = 0; j < TextureMapType::Count; j++)
		{
			auto mapType            = CAST(j, TextureMapType);
			const auto& [map, path] = GetMaterialTexture(directory, material, GetAssimpTextureType(mapType), mapType);

			if (map.m_Handle != AssetHandle::BAD)
			{
				entry.m_TextureMaps.push_back(std::move(map));
				const auto loadInfo = AssetLoadInfo{.m_Path = path, .m_AssetType = AssetType::Texture};
				if (std::ranges::find(texturesToLoad.begin(), texturesToLoad.end(), loadInfo) == texturesToLoad.end())
				{
					texturesToLoad.push_back(loadInfo);
				}
			}
		}
		model->m_Data.m_Materials.push_back(std::move(entry));
	}

	return texturesToLoad;
}
