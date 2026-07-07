#pragma once

#include "Core/STL.hpp"
#include "Render/Mesh.hpp"

namespace axm {

    struct CPUMesh
    {
        void* m_CPUMemory;
    };

    class Model
    {
    public:
        static Model CreateModelFromFile(const String& path);
    };
} // namespace axm
