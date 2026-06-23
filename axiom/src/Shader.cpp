#include <array>
#include "Shader.hpp"
#include "Log.hpp"

using namespace rhi;

slang::IModule* GetModule(IDevice* device, const char* name) {
    ComPtr<slang::IBlob> diagnostics = {};

    slang::IModule * shaderModule = device->getSlangSession()->loadModule(name, diagnostics.writeRef());

    if (diagnostics) {
        AXM_LOG("Shader Compilation Messages: {}", static_cast<const char *>(diagnostics->getBufferPointer()));
    }

    if (!shaderModule) {
        AXM_LOG("Failed to compile shader : {}", name);
        return nullptr;
    }

    return shaderModule;
}

void CreateShaderProgram(IDevice* device, ShaderProgramDesc desc, ComPtr<IShaderProgram>& program, const char* name) {
    ComPtr<slang::IBlob> diagnostics = {};

    if (SLANG_FAILED(device->createShaderProgram(desc, program.writeRef(), diagnostics.writeRef()))) {
        AXM_LOG("Failed to create shader program : {}", name);
        if (diagnostics) {
            AXM_LOG("{}", static_cast<const char*>(diagnostics->getBufferPointer()));
        }
    }
}

axm::Shader::Shader(IDevice *device, const char *name, const char *vertexEntry, const char *fragEntry) :m_Name(name){

    slang::IModule * shaderModule = GetModule(device, name);

    if (!shaderModule) {
        AXM_LOG("Failed to compile shader : {}", name);
        return;
    }

    slang::IEntryPoint* vertexEntryPoint = nullptr;
    shaderModule->findEntryPointByName(vertexEntry, &vertexEntryPoint);
    slang::IEntryPoint* fragmentEntryPoint = nullptr;
    shaderModule->findEntryPointByName(fragEntry, &fragmentEntryPoint);

    std::array<slang::IComponentType*, 2> entryPoints = { vertexEntryPoint, fragmentEntryPoint };

    ShaderProgramDesc programDesc = {};
    programDesc.linkingStyle = LinkingStyle::SingleProgram;
    programDesc.slangEntryPoints = entryPoints.data();
    programDesc.slangEntryPointCount = entryPoints.size();
    programDesc.slangGlobalScope = shaderModule;

    CreateShaderProgram(device, programDesc, m_Program, name);
}
axm::Shader::Shader(rhi::IDevice *device, const char *name, const char *computeEntry) : m_Name(name) {
    slang::IModule * shaderModule = GetModule(device, name);

    if (!shaderModule) {
        AXM_LOG("Failed to compile shader : {}", name);
        return;
    }

    slang::IEntryPoint* computeEntryPoint = nullptr;
    shaderModule->findEntryPointByName(computeEntry, &computeEntryPoint);

    std::array<slang::IComponentType*, 1> entryPoint = { computeEntryPoint };

    ShaderProgramDesc programDesc = {};
    programDesc.linkingStyle = LinkingStyle::SingleProgram;
    programDesc.slangEntryPoints = entryPoint.data();
    programDesc.slangEntryPointCount = entryPoint.size();
    programDesc.slangGlobalScope = shaderModule;

    CreateShaderProgram(device, programDesc, m_Program, name);
}
