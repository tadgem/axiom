#include "../include/Core/Utils.hpp"
#include <fstream>
#include <iostream>
#include "Core/Profile.hpp"

rhi::ITexture* axm::Utils::CreateDepthTexture(rhi::IDevice* device, u32 w, u32 h, rhi::Format format) {
    using namespace rhi;
    PROFILE_SCOPE()
    TextureDesc depthDesc  = { };
    depthDesc.type         = TextureType::Texture2D;
    depthDesc.size.width   = w;
    depthDesc.size.height  = h;
    depthDesc.size.depth   = 1;
    depthDesc.arrayLength  = 1;
    depthDesc.mipCount     = 1;
    depthDesc.format       = Format::D32Float;
    depthDesc.usage        = TextureUsage::DepthStencil;
    depthDesc.defaultState = ResourceState::DepthWrite;
    depthDesc.label        = "Depth Texture";
    ITexture* tex;
    device->createTexture(depthDesc, nullptr, &tex);
    return tex;
}
axm::Vector<u8> axm::Utils::LoadBinaryFromPath(const Filesystem::path& path) {
    PROFILE_SCOPE()
    std::ifstream file { path.c_str(), std::ios::binary | std::ios::ate };
    auto          fileSize = file.tellg();
    file.seekg(std::ios::beg);

    Vector<u8> vec = { };
    vec.resize(fileSize);
    file.read(reinterpret_cast<char*>(std::data(vec)), fileSize);

    return std::move(vec);
}
