#include "../include/Core/STL.hpp"

namespace axm {
    str_hash HashString(const String &input) {
        static auto hasher = std::hash<String>();
        return hasher(input);
    }
} // namespace axm
