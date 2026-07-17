#include "Core/STL.hpp"
#include "Core/Profile.hpp"

namespace axm {
    thread_local auto g_StringHasher = std::hash<String>();

    str_hash          HashString(const String& input) {
        PROFILE_SCOPE()
        return g_StringHasher(input);
    }

    str_hash HashPath(const Filesystem::path& path) {
        PROFILE_SCOPE()
        auto pathStr = String(path.string());
        return g_StringHasher(pathStr);
    }
} // namespace axm
