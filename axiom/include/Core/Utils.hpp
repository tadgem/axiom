#pragma once
#include "STL.hpp"
#include "slang-rhi.h"

namespace axm {
    class Utils
    {
    public:
        static DynArray<u8> LoadBinaryFromPath(const Filesystem::path& path);
    };
}
