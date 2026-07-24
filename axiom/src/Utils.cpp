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
