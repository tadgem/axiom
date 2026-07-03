//
// Created by Liam on 21/06/2026.
//

#pragma once
#include "Prim.hpp"
#include "slang-rhi.h"
namespace axm {
    namespace buffer {
        rhi::ComPtr<rhi::IBuffer> CreateVertexBuffer(rhi::IDevice *device, u64 size, const void *data,
                                                     const char *label = "VertexBuffer");

        rhi::ComPtr<rhi::IBuffer> CreateIndexBuffer(rhi::IDevice *device, u64 size, const void *data,
                                                    const char *label = "IndexBuffer");


    } // namespace buffer
}