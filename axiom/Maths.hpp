#pragma once
#include "Prim.hpp"

namespace axm {

    template<typename _FltType>
    struct vec3_t {
        union {
            _FltType _[3];
            struct {
                _FltType x;
                _FltType y;
                _FltType z;
            };
        };
    };

    using vec3 = vec3_t<f32>;

}