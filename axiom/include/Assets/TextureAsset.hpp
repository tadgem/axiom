#pragma once
#include "Assets/AssetManager.hpp"
#include "Render/GPU.hpp"
#include "Render/Texture.hpp"
namespace axm {

    using TextureAsset          = AssetT<Texture, AssetType::Texture>;
    using TextureAssetTransient = AssetTransientT<Texture, CPUTextureData, AssetType::Texture>;

    class TextureAssetFactory : public AssetFactory
    {
    public:
        GPU& m_GPU;

        explicit TextureAssetFactory(GPU& gpu);

        NO_DISCARD AssetLoadResult LoadAsset(const Filesystem::path& path) const override;
        void                       UnloadAsset(Asset* asset) const override;
        void                       ProcessAssetTransient(AssetTransient* data) const override;
    };
}
