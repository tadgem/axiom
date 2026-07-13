#pragma once
#include "Assets/AssetManager.hpp"
#include "Render/Texture.hpp"
namespace axm {

    using TextureAsset          = AssetT<Texture, AssetType::Texture>;
    using TextureAssetTransient = AssetTransientT<Texture, CPUTextureData, AssetType::Texture>;

    class TextureAssetFactory : public AssetFactory
    {
    public:
        rhi::IDevice* m_Device;

        explicit TextureAssetFactory(rhi::IDevice* gpuDevice);

        NO_DISCARD AssetLoadResult LoadAsset(const String& path) const override;
        void                       UnloadAsset(Asset* asset) const override;
        void                       ProcessAssetTransient(AssetTransient* data) const override;


    public:
    };
}
