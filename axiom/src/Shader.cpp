#include "Render/Shader.hpp"
#include "Core/Debug.hpp"

axm::Shader::Shader(vku::VkState& vk, const String& vertPath, const String& fragPath) :
    m_VertexPath(vertPath), m_FragmentPath(fragPath), m_IsCompute(false) {
    m_Program = vku::ShaderProgram::CreateGraphicsFromSourcePath(
            vk, m_VertexPath.c_str(), m_FragmentPath.c_str());
    m_Valid = !m_Program.m_Stages.empty();
    if (!m_Valid) {
        AXM_LOG_ERROR("Failed to compile graphics shader '{}' / '{}'", m_VertexPath, m_FragmentPath);
    }
}

axm::Shader::Shader(vku::VkState& vk, const String& computePath) :
    m_ComputePath(computePath), m_IsCompute(true) {
    m_Program = vku::ShaderProgram::CreateComputeFromSourcePath(vk, m_ComputePath.c_str());
    m_Valid   = !m_Program.m_Stages.empty();
    if (!m_Valid) {
        AXM_LOG_ERROR("Failed to compile compute shader '{}'", m_ComputePath);
    }
}

void axm::Shader::Free(vku::VkState& vk) {
    if (m_Valid) {
        m_Program.Free(vk);
        m_Valid = false;
    }
}

bool axm::Shader::Reload(vku::VkState& vk) {
    vku::ShaderProgram rebuilt;
    if (m_IsCompute) {
        rebuilt = vku::ShaderProgram::CreateComputeFromSourcePath(vk, m_ComputePath.c_str());
    } else {
        rebuilt = vku::ShaderProgram::CreateGraphicsFromSourcePath(
                vk, m_VertexPath.c_str(), m_FragmentPath.c_str());
    }

    if (rebuilt.m_Stages.empty()) {
        AXM_LOG_ERROR("Shader reload failed, keeping previous program");
        return false;
    }

    m_Program.Free(vk);
    m_Program = rebuilt;
    m_Valid   = true;
    return true;
}
