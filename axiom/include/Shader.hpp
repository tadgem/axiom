#pragma once
#include "slang-rhi.h"
#include "slang.h"

namespace axm {

    class Shader {
    public:

        explicit Shader(
            rhi::IDevice* device,
            const char* name,
            const char* vertexEntry = "vertexMain",
            const char* fragEntry = "fragmentMain"
        );

        explicit Shader(
            rhi::IDevice* device,
            const char* name,
            const char* computeEntry
        );

        rhi::ComPtr<rhi::IShaderProgram> m_Program;
    };

}