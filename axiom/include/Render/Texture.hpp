#pragma once

#include "../Core/Prim.hpp"
#include "slang-rhi.h"

namespace axm {
    struct Texture {
        rhi::ITexture *m_GPUTexture;
        rhi::ITextureView *m_TextureView;

        static Texture BAD();
    };

    struct CPUTextureData {
        void *m_Data;
        int m_Width, m_Height, m_NumChannels;

        void Release() const;
    };

    namespace textures {

        CPUTextureData LoadCPUTextureDataFromMemory(void *data, size_t length);
        CPUTextureData LoadCPUTextureDataFromFile(const char *path);

        rhi::ComPtr<rhi::ISampler> CreateSampler(rhi::IDevice *device, rhi::TextureFilteringMode filter,
                                                 rhi::TextureAddressingMode addressMode);

        Texture CreateTexture2D(rhi::IDevice *device, const void *data, rhi::Format format, u32 w, u32 h);
    } // namespace textures

} // namespace axm
