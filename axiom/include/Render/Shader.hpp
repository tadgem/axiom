#pragma once
#include "Core/Debug.hpp"
#include "Core/Profile.hpp"
#include "Core/STL.hpp"
#include "slang-rhi.h"
#include "slang-rhi/shader-cursor.h"

namespace axm {

    namespace shaders {
        void            CreateShaderProgram(rhi::IDevice*                     device,
                                            rhi::ShaderProgramDesc            desc,
                                            rhi::ComPtr<rhi::IShaderProgram>& program,
                                            const String&                     name);

        slang::IModule* GetModule(rhi::IDevice* device, const char* name);
    }
    class Shader
    {
    public:
        Shader() = default;

        explicit Shader(rhi::IDevice* device, const String& name, const String& computeEntry);
        explicit Shader(rhi::IDevice* device, const String& name, const String& vertEntry, const String& fragEntry);
        explicit Shader(rhi::IDevice* device, const String& name, const Span<String>& entries);

        String                           m_Name;
        rhi::ComPtr<rhi::IShaderProgram> m_Program;
    };

    // Short-lived object, wrapper around shader cursor
    // which allows reflected interaction with shader data
    class ShaderDataInterface
    {
    public:
        rhi::ShaderCursor m_SlangCursor;
        const String&     m_PipelineName;

        ShaderDataInterface(rhi::IShaderObject* obj, const String& pipelineName = "Unknown Pipeline");

        template <typename T>
        void SetData(const char* bindingName, const T& data) const {
            PROFILE_SCOPE()
            if (m_SlangCursor[bindingName].setData(&data, sizeof(T)) < 0) {
                AXM_LOG("Failed to set data of type {} at binding {} to pipeline {} ",
                        typeid(T).name(),
                        bindingName,
                        m_PipelineName);
            }
        }

        void SetBinding(const char* bindingName, const rhi::Binding& binding) const {
            PROFILE_SCOPE()
            if (m_SlangCursor[bindingName].setBinding(binding) < 0) {
                AXM_LOG("Failed to bind {} to pipeline {} ", bindingName, m_PipelineName);
            }
        }
    };

    namespace shaders { }

}
