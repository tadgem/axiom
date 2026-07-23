#pragma once

#include "Core/STL.hpp"
#include "Render/GPU.hpp"

namespace axm {

    class Shader;

    enum TextureMapType {
        Unknown = -1,
        Diffuse,
        Normal,
        Specular,
        Metallic,
        Opacity,
        Height,
        Displacement,
        Roughness,
        AO,
        Emissive,
        Count
    };

    struct Texture
    {
        rhi::ITexture*     m_GPUTexture;
        rhi::ITextureView* m_TextureView;

        static Texture     BAD();
    };

    struct CPUTextureData
    {
        void* m_Data;
        int   m_Width, m_Height, m_NumChannels;

        void  Release() const;
    };

    namespace textures {

        CPUTextureData LoadCPUTextureDataFromMemory(void* data, size_t length);
        CPUTextureData LoadCPUTextureDataFromFile(const Filesystem::path& path);

        rhi::ComPtr<rhi::ISampler>
        CreateSampler(rhi::IDevice* device, rhi::TextureFilteringMode filter, rhi::TextureAddressingMode addressMode);

        Texture
        CreateTexture2D(GPU& device, const void* data, rhi::Format format, u32 w, u32 h, const char* label = "UNKNOWN");

        void GenerateMips(GPU& device, Texture& texture);

    } // namespace textures

} // namespace axm
