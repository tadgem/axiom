#pragma once
#include "Assets/AssetManager.hpp"
#include "Render/Texture.hpp"
namespace axm {

    using TextureAsset          = AssetT<Texture, AssetType::Texture>;
    using TextureAssetTransient = AssetTransientT<Texture, CPUTextureData, AssetType::Texture>;

    class TextureAssetFactory : public AssetFactory
    {
    public:
        rhi::ComPtr<rhi::IDevice> m_Device;

        TextureAssetFactory(rhi::ComPtr<rhi::IDevice> gpuDevice);

        AssetLoadResult LoadAsset(const String& path) const override;
        void            UnloadAsset(Asset* asset) const override;
        void            ProcessAssetTransient(AssetTransientData* data, u16 step) const override;


    public:
    };
}
