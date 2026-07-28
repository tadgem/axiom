#pragma once
#include "Maths.hpp"
#include "STL.hpp"

namespace axm {
    class Utils
    {
    public:
        static DynArray<u8> LoadBinaryFromPath(const Filesystem::path& path);
        static aml::Mat44   CreateModelMatrix(const aml::Vec3& pos, const aml::Vec3& euler, const aml::Vec3& scale);
        static aml::Mat44   CreateViewMatrix(const aml::Vec3& pos, const aml::Vec3& euler);
    };
}
