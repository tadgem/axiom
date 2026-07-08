#include "Render/Texture.hpp"
#include "Core/Debug.hpp"
#include "Core/Profile.hpp"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

axm::Texture axm::textures::CreateTexture2D(rhi::IDevice* device, const void* data, rhi::Format format, u32 w, u32 h) {
    PROFILE_SCOPE();

    rhi::TextureDesc textureDesc = { };
    textureDesc.type = rhi::TextureType::Texture2D;
    textureDesc.size.width = w;
    textureDesc.size.height = h;
    textureDesc.size.depth = 1;
    textureDesc.arrayLength = 1;
    textureDesc.mipCount = 1;
    textureDesc.format = format;
    textureDesc.usage = rhi::TextureUsage::ShaderResource;
    textureDesc.defaultState = rhi::ResourceState::ShaderResource;
    textureDesc.label = "ImGui Font Texture";

    auto formatInfo = rhi::getRHI()->getFormatInfo(format);

    rhi::SubresourceData initData = { };
    initData.data = data;
    initData.rowPitch = w * 4;

    Texture tex = { };

    if (SLANG_FAILED(device->createTexture(textureDesc, &initData, &tex.m_GPUTexture))) {
        AXM_LOG("Failed to create texture");
        return Texture::BAD();
    }

    tex.m_TextureView = tex.m_GPUTexture->getDefaultView();
    if (!tex.m_TextureView) {
        AXM_LOG("Failed to acquire texure view");
        return Texture::BAD();
    }

    return tex;
}
axm::Texture axm::Texture::BAD() { return { .m_GPUTexture = nullptr, .m_TextureView = nullptr }; }
void axm::CPUTextureData::Release() const { stbi_image_free(m_Data); }
axm::CPUTextureData axm::textures::LoadCPUTextureDataFromMemory(void* data, size_t length) {
    PROFILE_SCOPE();

    int texWidth, texHeight, texChannels;
    auto* pixels = stbi_load_from_memory(static_cast<stbi_uc const*>(data),
                                         static_cast<int>(length),
                                         &texWidth,
                                         &texHeight,
                                         &texChannels,
                                         STBI_rgb_alpha);

    return { .m_Data = pixels, .m_Width = texWidth, .m_Height = texHeight, .m_NumChannels = texChannels };
}
axm::CPUTextureData axm::textures::LoadCPUTextureDataFromFile(const char* path) {
    PROFILE_SCOPE();
    int texWidth, texHeight, texChannels;
    auto* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    return { .m_Data = pixels, .m_Width = texWidth, .m_Height = texHeight, .m_NumChannels = texChannels };
}
rhi::ComPtr<rhi::ISampler> axm::textures::CreateSampler(rhi::IDevice* device,
                                                        rhi::TextureFilteringMode filter,
                                                        rhi::TextureAddressingMode addressMode) {
    PROFILE_SCOPE();

    using namespace rhi;
    SamplerDesc samplerDesc = { };
    samplerDesc.minFilter = filter;
    samplerDesc.magFilter = filter;
    samplerDesc.mipFilter = filter;
    samplerDesc.addressU = addressMode;
    samplerDesc.addressV = addressMode;
    samplerDesc.addressW = addressMode;

    ComPtr<ISampler> sampler = { };

    if (SLANG_FAILED(device->createSampler(samplerDesc, sampler.writeRef()))) {
        AXM_LOG("Failed to create sampler!");
        return nullptr;
    }

    return sampler;
}
