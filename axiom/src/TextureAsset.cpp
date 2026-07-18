#include "Assets/TextureAsset.hpp"
#include "Core/Utils.hpp"

void OnProcessTextureAssetTransient(axm::AssetTransient* data) { using namespace axm; }

axm::TextureAssetFactory::TextureAssetFactory(rhi::IDevice* gpuDevice) :
    AssetFactory(AssetType::Texture), m_Device(gpuDevice) { }

axm::AssetLoadResult axm::TextureAssetFactory::LoadAsset(const Filesystem::path& path) const {
    auto            cpuTexture   = textures::LoadCPUTextureDataFromFile(path.c_str());

    AssetLoadResult result       = { };

    auto*           textureAsset = AXM_NEW(TextureAsset, String(path.string()), { });
    auto*           transient    = AXM_NEW(TextureAssetTransient, textureAsset, std::move(cpuTexture));

    // TODO: Can we enforce this as a compile error?
    // SPECIFICALLY, we need to downcast a AssetTransient<T> to a AssetTransient*
    // so that the code that inspects on the variant (m_Next) does not fall apart.

    transient->m_NumSteps = 1;
    result.m_Next         = dynamic_cast<AssetTransient*>(transient);
    return result;
}
void axm::TextureAssetFactory::UnloadAsset(Asset* asset) const { }
void axm::TextureAssetFactory::ProcessAssetTransient(AssetTransient* data) const {
    auto*         transient = dynamic_cast<TextureAssetTransient*>(data);
    TextureAsset* tex       = transient->GetConcreteAsset();

    tex->m_Data             = textures::CreateTexture2D(m_Device,
                                            transient->m_TransientData.m_Data,
                                            rhi::Format::RGBA8UnormSrgb,
                                            transient->m_TransientData.m_Width,
                                            transient->m_TransientData.m_Height);
    transient->m_CurrentStep++;
}
