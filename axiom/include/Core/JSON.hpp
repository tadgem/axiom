#pragma once

#include "STL.hpp"
#include "nlohmann_json.hpp"

namespace axm {
    using json = nlohmann::basic_json<std::map, std::vector, String, bool, i64, u64, f64, STLMimallocAllocator,
                                      nlohmann::adl_serializer, Vector<u8>, void>;
}