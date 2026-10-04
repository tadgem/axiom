#pragma once

#include "Core/Prim.hpp"
#include "Core/STL.hpp"
#include "vku/vku.h"

namespace axm {

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
        vku::Texture m_Texture;
        bool         m_Valid = false;

        NO_DISCARD VkImageView View() const { return m_Texture.m_ImageView; }
        NO_DISCARD VkSampler   Sampler() const { return m_Texture.m_Sampler; }
        NO_DISCARD VkFormat    Format() const { return m_Texture.m_Format; }

        void                   Free(vku::VkState& vk) { m_Texture.Free(vk); }

        static Texture         BAD();
    };

    struct CPUTextureData
    {
        void* m_Data = nullptr;
        u32   m_Width = 0, m_Height = 0, m_NumChannels = 0;

        void  Release() const;
    };

    namespace textures {

        CPUTextureData LoadCPUTextureDataFromMemory(void* data, size_t length);
        CPUTextureData LoadCPUTextureDataFromFile(const Filesystem::path& path);

        Texture        CreateTexture2D(vku::VkState&     vk,
                                       const void*       data,
                                       VkFormat          format,
                                       u32               w,
                                       u32               h,
                                       const char*       label = "UNKNOWN");

    } // namespace textures

} // namespace axm
