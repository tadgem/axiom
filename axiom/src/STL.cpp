#include "Core/STL.hpp"
#include "Core/Profile.hpp"

namespace axm {
    str_hash HashString(const String& input) {
        PROFILE_SCOPE()
        static auto hasher = std::hash<String>();
        return hasher(input);
    }
} // namespace axm
