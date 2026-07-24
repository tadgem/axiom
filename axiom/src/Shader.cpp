#include "Render/Shader.hpp"
#include "Core/Debug.hpp"
#include "Core/Profile.hpp"
#include "Core/STL.hpp"

using namespace rhi;

slang::IModule* axm::shaders::GetModule(IDevice* device, const char* name) {
    PROFILE_SCOPE()

    ComPtr<slang::IBlob> diagnostics  = { };

    slang::IModule*      shaderModule = device->getSlangSession()->loadModule(name, diagnostics.writeRef());

    if (diagnostics) {
        AXM_LOG("Shader Compilation Messages: {}", static_cast<const char*>(diagnostics->getBufferPointer()));
    }

    if (!shaderModule) {
        AXM_LOG("Failed to compile shader : {}", name);
        return nullptr;
    }

    return shaderModule;
}

void axm::shaders::CreateShaderProgram(IDevice*                device,
                                       ShaderProgramDesc       desc,
                                       ComPtr<IShaderProgram>& program,
                                       const axm::String&      name) {
    PROFILE_SCOPE()

    ComPtr<slang::IBlob> diagnostics = { };

    if (SLANG_FAILED(device->createShaderProgram(desc, program.writeRef(), diagnostics.writeRef()))) {
        AXM_LOG("Failed to create shader program : {}", name);
        if (diagnostics) {
            AXM_LOG("{}", static_cast<const char*>(diagnostics->getBufferPointer()));
        }
    }
}


axm::ShaderDataInterface::ShaderDataInterface(IShaderObject* obj, const String& pipelineName) :
    m_SlangCursor(ShaderCursor(obj)), m_PipelineName(pipelineName) { }
