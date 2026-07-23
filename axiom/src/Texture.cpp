#include "Render/Texture.hpp"
#include "Core/Debug.hpp"
#include "Core/Profile.hpp"
#include "Render/Pipeline.hpp"
#include "Render/Shader.hpp"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

axm::Texture
axm::textures::CreateTexture2D(GPU& gpu, const void* data, rhi::Format format, u32 w, u32 h, const char* label) {
    PROFILE_SCOPE()

    uint32_t mips                          = 1;
    mips                                   = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;

    rhi::TextureDesc textureDesc           = { };
    textureDesc.type                       = rhi::TextureType::Texture2D;
    textureDesc.size.width                 = w;
    textureDesc.size.height                = h;
    textureDesc.size.depth                 = 1;
    textureDesc.arrayLength                = 1;
    textureDesc.mipCount                   = mips;
    textureDesc.format                     = format;
    textureDesc.usage                      = rhi::TextureUsage::ShaderResource | rhi::TextureUsage::UnorderedAccess;
    textureDesc.defaultState               = rhi::ResourceState::ShaderResource;
    textureDesc.label                      = label;

    Vector<rhi::SubresourceData> initDatas = { };

    initDatas.push_back({ .data = data, .rowPitch = w * 4 });
    for (auto i = 0; i < mips; i++) {
        auto mipWidth = w / (2 * (i + 1));
        initDatas.push_back({ .data = data, .rowPitch = mipWidth * 4 });
    }

    Texture tex = { };

    if (SLANG_FAILED(gpu.m_Device->createTexture(textureDesc, initDatas.data(), &tex.m_GPUTexture))) {
        AXM_LOG("Failed to create texture.");
        return Texture::BAD();
    }

    tex.m_TextureView = tex.m_GPUTexture->getDefaultView();
    if (!tex.m_TextureView) {
        AXM_LOG("Failed to acquire texture view");
        return Texture::BAD();
    }

    if (mips > 1) {
        GenerateMips(gpu, tex);
    }

    return tex;
}

void axm::textures::GenerateMips(GPU& gpu, Texture& texture) {
    PROFILE_SCOPE()

    if (!texture.m_GPUTexture || !gpu.m_Device) {
        AXM_LOG_ERROR("Cannot generate mips for invalid texture or device.");
        return;
    }

    const auto desc = texture.m_GPUTexture->getDesc();
    if (desc.mipCount <= 1) {
        return;
    }
    if (!gpu.m_MipPipeline) {
        AXM_LOG_ERROR("Failed to create compute pipeline for generating mips.");
        return;
    }

    auto commandEncoder = gpu.m_Queue->createCommandEncoder();

    if (!commandEncoder) {
        AXM_LOG_ERROR("Failed to create command encoder for mip generation");
        return;
    }
    struct MipParams
    {
        f32 srcTexelSize[2];
        u32 dstSize[2];
        u32 srcMipLevel;
    };

    for (u32 m = 1; m < desc.mipCount; ++m) {
        u32                  srcWidth              = std::max(1u, desc.size.width >> (m - 1));
        u32                  srcHeight             = std::max(1u, desc.size.height >> (m - 1));
        u32                  dstWidth              = std::max(1u, desc.size.width >> m);
        u32                  dstHeight             = std::max(1u, desc.size.height >> m);

        rhi::TextureViewDesc srcViewDesc           = { };
        srcViewDesc.format                         = desc.format;
        srcViewDesc.subresourceRange.layer         = 0;
        srcViewDesc.subresourceRange.layerCount    = desc.arrayLength;
        srcViewDesc.subresourceRange.mip           = m - 1;
        srcViewDesc.subresourceRange.mipCount      = 1;

        rhi::ComPtr<rhi::ITextureView> srcView     = texture.m_GPUTexture->createView(srcViewDesc);

        rhi::TextureViewDesc           dstViewDesc = { };
        dstViewDesc.format                         = desc.format;
        dstViewDesc.subresourceRange.layer         = 0;
        dstViewDesc.subresourceRange.layerCount    = desc.arrayLength;
        dstViewDesc.subresourceRange.mip           = m;
        dstViewDesc.subresourceRange.mipCount      = 1;

        rhi::ComPtr<rhi::ITextureView> dstView     = texture.m_GPUTexture->createView(dstViewDesc);

        if (!srcView || !dstView) {
            AXM_LOG("Failed to create texture views for mip generation level {}", m);
            commandEncoder->finish();
            return;
        }

        auto computePass = commandEncoder->beginComputePass();
        if (!computePass) {
            AXM_LOG("Failed to begin compute pass for mip generation level {}", m);
            commandEncoder->finish();

            return;
        }

        rhi::ShaderCursor cursor(computePass->bindPipeline(gpu.m_MipPipeline));

        MipParams params = { .srcTexelSize = { 1.0f / static_cast<f32>(srcWidth), 1.0f / static_cast<f32>(srcHeight) },
                             .dstSize      = { dstWidth, dstHeight },
                             .srcMipLevel  = 0 };

        cursor["params"].setData(&params, sizeof(params));
        cursor["srcTexture"].setBinding(srcView);
        cursor["srcSampler"].setBinding(gpu.m_LinearClampSampler);
        cursor["dstTexture"].setBinding(dstView);

        u32 groupsX = (dstWidth + 7) / 8;
        u32 groupsY = (dstHeight + 7) / 8;

        computePass->dispatchCompute(groupsX, groupsY, 1);
        computePass->end();
    }

    auto commandBuffer = commandEncoder->finish();
    gpu.m_Queue->submit(commandBuffer);
}

axm::Texture        axm::Texture::BAD() { return { .m_GPUTexture = nullptr, .m_TextureView = nullptr }; }
void                axm::CPUTextureData::Release() const { stbi_image_free(m_Data); }
axm::CPUTextureData axm::textures::LoadCPUTextureDataFromMemory(void* data, size_t length) {
    PROFILE_SCOPE()
    // stbi_set_flip_vertically_on_load(true);
    int   texWidth, texHeight, texChannels;

    auto* pixels = stbi_load_from_memory(static_cast<stbi_uc const*>(data),
                                         static_cast<int>(length),
                                         &texWidth,
                                         &texHeight,
                                         &texChannels,
                                         STBI_rgb_alpha);

    return { .m_Data = pixels, .m_Width = texWidth, .m_Height = texHeight, .m_NumChannels = texChannels };
}
axm::CPUTextureData axm::textures::LoadCPUTextureDataFromFile(const Filesystem::path& path) {
    PROFILE_SCOPE()
    auto newPath = path.u8string();
    int  texWidth, texHeight, texChannels;
    stbi_set_flip_vertically_on_load(true);
    auto* pixels = stbi_load(
            reinterpret_cast<const char*>(newPath.c_str()), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    return { .m_Data = pixels, .m_Width = texWidth, .m_Height = texHeight, .m_NumChannels = texChannels };
}
rhi::ComPtr<rhi::ISampler> axm::textures::CreateSampler(rhi::IDevice*              device,
                                                        rhi::TextureFilteringMode  filter,
                                                        rhi::TextureAddressingMode addressMode) {
    PROFILE_SCOPE()

    using namespace rhi;
    SamplerDesc samplerDesc  = { };
    samplerDesc.minFilter    = filter;
    samplerDesc.magFilter    = filter;
    samplerDesc.mipFilter    = filter;
    samplerDesc.addressU     = addressMode;
    samplerDesc.addressV     = addressMode;
    samplerDesc.addressW     = addressMode;

    ComPtr<ISampler> sampler = { };

    if (SLANG_FAILED(device->createSampler(samplerDesc, sampler.writeRef()))) {
        AXM_LOG("Failed to create sampler!");
        return nullptr;
    }

    return sampler;
}
