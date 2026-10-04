#pragma once
#include "Core/STL.hpp"
#include "vku/vku.h"

namespace axm {

    // Owns a vku::ShaderProgram compiled from GLSL source. Source is compiled at
    // runtime (shaderc), so shader text files can be edited and reloaded without
    // rebuilding the application.
    class Shader
    {
    public:
        Shader() = default;

        Shader(vku::VkState& vk, const String& vertPath, const String& fragPath);
        Shader(vku::VkState& vk, const String& computePath);

        NO_DISCARD bool Valid() const { return m_Valid; }

        // Recompiles from the recorded paths into a new program. On failure the
        // existing program is kept and false is returned.
        bool            Reload(vku::VkState& vk);

        vku::ShaderProgram m_Program;
        bool               m_Valid      = false;
        bool               m_IsCompute  = false;
        String             m_VertexPath;
        String             m_FragmentPath;
        String             m_ComputePath;
    };

} // namespace axm
