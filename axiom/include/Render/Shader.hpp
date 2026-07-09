#pragma once
#include "../Core/Debug.hpp"
#include "slang-rhi.h"
#include "slang-rhi/shader-cursor.h"
#include "slang.h"

namespace axm {

    class Shader
    {
    public:
        explicit Shader(rhi::IDevice* device,
                        const char*   name,
                        const char*   vertexEntry = "vertexMain",
                        const char*   fragEntry   = "fragmentMain");

        explicit Shader(rhi::IDevice* device, const char* name, const char* computeEntry);

        const char*                      m_Name;
        rhi::ComPtr<rhi::IShaderProgram> m_Program;
    };

    // Short-lived object, wrapper around shader cursor
    // which allows reflected interaction with shader data
    class ShaderDataInterface
    {
    public:
        rhi::ShaderCursor                 m_SlangCursor;
        rhi::ComPtr<rhi::IRenderPipeline> m_RenderPipeline;

        ShaderDataInterface(rhi::IRenderPassEncoder*                 renderPassEncoder,
                            const rhi::ComPtr<rhi::IRenderPipeline>& pipeline);

        template <typename T>
        void SetData(const char* bindingName, const T& data) const {
            if (m_SlangCursor[bindingName].setData(&data, sizeof(T)) < 0) {
                AXM_LOG("Failed to set data of type {} at binding {} to pipeline with shader {}",
                        typeid(T).name(),
                        bindingName,
                        m_RenderPipeline->getDesc().label);
            }
        }

        void SetBinding(const char* bindingName, const rhi::Binding& binding) const {
            if (m_SlangCursor[bindingName].setBinding(binding) < 0) {
                AXM_LOG("Failed to bind {} to pipeline with shader {}", bindingName, m_RenderPipeline->getDesc().label);
            }
        }
    };

    namespace shaders { }

}
