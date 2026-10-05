#include "Render/Texture.hpp"
#include "Core/Debug.hpp"

#include <algorithm>
#include <cmath>

#include "stb_image.h"

axm::Texture axm::Texture::BAD() {
    return { .m_Texture = { }, .m_Valid = false };
}

axm::Texture axm::textures::CreateTexture2D(
        vku::VkState& vk, const void* data, VkFormat format, u32 w, u32 h, const char* label) {
    (void)label;

    Texture tex{};

    const u32 mips = static_cast<u32>(std::floor(std::log2(std::max(w, h)))) + 1;

    VkImage        image   = VK_NULL_HANDLE;
    VkDeviceMemory memory  = VK_NULL_HANDLE;
    VkImageView    view    = VK_NULL_HANDLE;
    VkSampler      sampler = VK_NULL_HANDLE;

    vku::textures::CreateImage(vk,
                               w,
                               h,
                               mips,
                               VK_SAMPLE_COUNT_1_BIT,
                               format,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                       | VK_IMAGE_USAGE_SAMPLED_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               image,
                               memory);
    vku::textures::CreateImageView(vk, image, format, mips, VK_IMAGE_ASPECT_COLOR_BIT, view);
    vku::textures::CreateImageSampler(
            vk, mips, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, sampler);

    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(w) * h * 4;

    vku::MappedBuffer staging = vku::buffers::CreateMappedBuffer(
            vk,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::memcpy(staging.m_MappedAddr, data, static_cast<size_t>(imageSize));

    vku::textures::TransitionImageLayout(vk,
                                         image,
                                         format,
                                         mips,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vku::textures::CopyBufferToImage(vk, staging.m_GpuBuffer, image, w, h);
    vku::textures::GenerateMips(vk, image, format, w, h, mips, VK_FILTER_LINEAR);
    vku::textures::TransitionImageLayout(vk,
                                         image,
                                         format,
                                         mips,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    staging.Free(vk);

    tex.m_Texture = vku::Texture(image, view, memory, sampler, format, VK_SAMPLE_COUNT_1_BIT);
    tex.m_Valid   = true;
    return tex;
}

void axm::CPUTextureData::Release() const { stbi_image_free(m_Data); }

axm::CPUTextureData axm::textures::LoadCPUTextureDataFromMemory(void* data, size_t length) {
    int texWidth = 0, texHeight = 0, texChannels = 0;

    auto* pixels = stbi_load_from_memory(CAST(data, stbi_uc const*),
                                         CAST(length, int),
                                         &texWidth,
                                         &texHeight,
                                         &texChannels,
                                         STBI_rgb_alpha);

    return { .m_Data        = pixels,
             .m_Width       = CAST(texWidth, u32),
             .m_Height      = CAST(texHeight, u32),
             .m_NumChannels = CAST(texChannels, u32) };
}

axm::CPUTextureData axm::textures::LoadCPUTextureDataFromFile(const Filesystem::path& path) {
    auto newPath = path.generic_string();
    int  texWidth = 0, texHeight = 0, texChannels = 0;
    stbi_set_flip_vertically_on_load(true);
    auto* pixels = stbi_load(
            reinterpret_cast<const char*>(newPath.c_str()), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    return { .m_Data        = pixels,
             .m_Width       = CAST(texWidth, u32),
             .m_Height      = CAST(texHeight, u32),
             .m_NumChannels = CAST(texChannels, u32) };
}
