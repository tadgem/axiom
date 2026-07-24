#include "../include/Core/Utils.hpp"
#include <fstream>
#include <iostream>
#include "Core/Profile.hpp"

axm::DynArray<u8> axm::Utils::LoadBinaryFromPath(const Filesystem::path& path) {
    PROFILE_SCOPE()
    std::ifstream file { path.c_str(), std::ios::binary | std::ios::ate };
    auto          fileSize = file.tellg();
    file.seekg(std::ios::beg);

    DynArray<u8> vec = { };
    vec.resize(fileSize);
    file.read(reinterpret_cast<char*>(std::data(vec)), fileSize);

    return std::move(vec);
}
JPH::Mat44 axm::Utils::CreateModelMatrix(const aml::Vec3& pos, const aml::Vec3& euler, const aml::Vec3& scale) {
    const auto eulerRadians = aml::Vec3 { aml::DegreesToRadians(euler.GetX()),
                                          aml::DegreesToRadians(euler.GetY()),
                                          aml::DegreesToRadians(euler.GetZ()) };
    aml::Mat44 m            = aml::Mat44::sRotationTranslation(aml::Quat::sEulerAngles(eulerRadians), pos);
    m                       = m.PreScaled(scale);
    return m;
}
