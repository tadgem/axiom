#include "../include/Render/Shader.hpp"
#include <array>
#include "../include/Core/Debug.hpp"
#include "../include/Core/STL.hpp"
#include "Core/Profile.hpp"

using namespace rhi;

slang::IModule* GetModule(IDevice* device, const char* name) {
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

static void
CreateShaderProgram(IDevice* device, ShaderProgramDesc desc, ComPtr<IShaderProgram>& program, const axm::String& name) {
    PROFILE_SCOPE()

    ComPtr<slang::IBlob> diagnostics = { };

    if (SLANG_FAILED(device->createShaderProgram(desc, program.writeRef(), diagnostics.writeRef()))) {
        AXM_LOG("Failed to create shader program : {}", name);
        if (diagnostics) {
            AXM_LOG("{}", static_cast<const char*>(diagnostics->getBufferPointer()));
        }
    }
}


axm::Shader::Shader(IDevice* device, const String& name, Span<String> entries) : m_Name(name) {
    PROFILE_SCOPE()

    slang::IModule* shaderModule = GetModule(device, name.c_str());

    if (!shaderModule) {
        AXM_LOG("Failed to compile shader : {}", name);
        return;
    }

    Vector<slang::IComponentType*> entryPoints = { };

    for (const auto& entry: entries) {
        slang::IEntryPoint* ep = { };
        shaderModule->findEntryPointByName(entry.c_str(), &ep);

        if (ep) {
            entryPoints.push_back(ep);
        }
    }

    ShaderProgramDesc programDesc    = { };
    programDesc.linkingStyle         = LinkingStyle::SingleProgram;
    programDesc.slangEntryPoints     = entryPoints.data();
    programDesc.slangEntryPointCount = entryPoints.size();
    programDesc.slangGlobalScope     = shaderModule;

    CreateShaderProgram(device, programDesc, m_Program, name);
}
axm::ShaderDataInterface::ShaderDataInterface(IRenderPassEncoder*            renderPassEncoder,
                                              const ComPtr<IRenderPipeline>& pipeline) :
    m_SlangCursor(rhi::ShaderCursor(renderPassEncoder->bindPipeline(pipeline))), m_RenderPipeline(pipeline) { }
