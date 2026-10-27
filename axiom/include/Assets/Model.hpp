#pragma once

#include "Assets/AssetManager.hpp"
#include "Core/STL.hpp"
#include "Render/Mesh.hpp"
#include "Render/Texture.hpp"
#include "assimp/IOStream.hpp"
#include "assimp/IOSystem.hpp"
#include "assimp/Importer.hpp"
#include "vku/vku.h"

struct aiScene;

namespace axm
{

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

	struct StaticMeshEntry
	{
		GPUMesh m_Mesh;
		u32     m_MaterialIndex;
	};

	struct SkeletalMeshEntry
	{
		GPUMesh m_Mesh;
		u32     m_MaterialIndex;
	};

	DynArray<StaticMeshEntry>   m_Meshes;
	DynArray<SkeletalMeshEntry> m_SkeletalMeshes;
	DynArray<MaterialEntry>     m_Materials;
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
	vku::VkState& m_VK;

	explicit ModelAssetFactory(vfspp::VirtualFileSystem& vfs, vku::VkState& vk);

	NO_DISCARD AssetLoadResult     LoadAsset(const Filesystem::path& path) const override;
	void                           UnloadAsset(Asset* asset) const override;
	void                           ProcessAssetTransient(AssetTransient* data) const override;
	void                           OnTransientComplete(AssetTransient* data) const override;

	static DynArray<AssetLoadInfo> ProcessSceneMaterials(const String&  directory,
	                                                     const aiScene* scene,
	                                                     ModelAsset*    model);
};

}  // namespace axm
