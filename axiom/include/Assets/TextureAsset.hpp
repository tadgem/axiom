#pragma once
#include "Assets/AssetManager.hpp"
#include "Render/Texture.hpp"
#include "vku/vku.h"
namespace axm
{

using TextureAsset          = AssetT<Texture, AssetType::Texture>;
using TextureAssetTransient = AssetTransientT<Texture, CPUTextureData, AssetType::Texture>;

class TextureAssetFactory : public AssetFactory
{
public:
	vku::VkState& m_VK;

	explicit TextureAssetFactory(vfspp::VirtualFileSystem& vfs, vku::VkState& vk);

	NO_DISCARD AssetLoadResult LoadAsset(const Filesystem::path& path) const override;
	void                       UnloadAsset(Asset* asset) const override;
	void                       ProcessAssetTransient(AssetTransient* data) const override;
	void                       OnTransientComplete(AssetTransient* data) const override;
};
}  // namespace axm
