#include "Assets/TextureAsset.hpp"
#include "Core/Utils.hpp"

void OnProcessTextureAssetTransient(axm::AssetTransientData* data) { using namespace axm; }

axm::TextureAssetFactory::TextureAssetFactory(rhi::ComPtr<rhi::IDevice> gpuDevice) :
    AssetFactory(AssetType::Texture, AXM_HAS_TRANSIENT), m_Device(gpuDevice) { }

axm::AssetLoadResult axm::TextureAssetFactory::LoadAsset(const String& path) const {
    auto            textureBinary = Utils::LoadBinaryFromPath(path);
    auto            cpuTexture    = textures::LoadCPUTextureDataFromMemory(textureBinary.data(), textureBinary.size());

    AssetLoadResult result        = { };

    TextureAsset*   textureAsset  = AXM_NEW(TextureAsset, path, { });
    result.m_Next                 = AXM_NEW(TextureAssetTransient, textureAsset, std::move(cpuTexture));
    // result.m_SyncAssetCallbacks.push_back()
    return { };
}
void axm::TextureAssetFactory::UnloadAsset(Asset* asset) const { }
void axm::TextureAssetFactory::ProcessAssetTransient(AssetTransientData* data, u16 step) const {
    TextureAssetTransient* transient = static_cast<TextureAssetTransient*>(data);
    TextureAsset*          tex       = transient->GetConcreteAsset();

    tex->m_Data                      = textures::CreateTexture2D(m_Device,
                                            transient->m_TransientData.m_Data,
                                            rhi::Format::RGBA8Unorm,
                                            transient->m_TransientData.m_Width,
                                            transient->m_TransientData.m_Height);
}
