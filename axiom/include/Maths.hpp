#pragma once
#include "Prim.hpp"

namespace axm {

    template<typename DecimalType>
    struct vec2_t {
        union {
            DecimalType _[2];
            struct {
                DecimalType x;
                DecimalType y;
            };
        };
    };

    template<typename DecimalType>
    struct vec3_t {
        union {
            DecimalType _[3];
            struct {
                DecimalType x;
                DecimalType y;
                DecimalType z;
            };
        };
    };

    template<typename DecimalType>
    struct vec4_t {
        union {
            DecimalType _[4];
            struct {
                DecimalType x;
                DecimalType y;
                DecimalType z;
                DecimalType w;
            };
        };
    };

    using vec2 = vec2_t<f32>;
    using vec3 = vec3_t<f32>;
    using vec4 = vec4_t<f32>;

    template<typename DecimalType>
    struct mat4x4_t {
        DecimalType m[4][4];

        static mat4x4_t Identity() {
            mat4x4_t m = {};
            m.m[0][0] = 1.0f;
            m.m[1][1] = 1.0f;
            m.m[2][2] = 1.0f;
            m.m[3][3] = 1.0f;
            return m;
        }
    };

    using mat4 = mat4x4_t<f32>;


    namespace maths {

        constexpr f64 kPI = 3.14159265358979323846f;

        template<typename DecimalType>
        DecimalType Radians(DecimalType deg) {
            return deg * kPI / 180.0f;
        }

        template<typename DecimalType>
        mat4x4_t<DecimalType> Multiply(const mat4x4_t<DecimalType>& a, const mat4x4_t<DecimalType>& b) {
            mat4x4_t<DecimalType> m = {};

            for (auto i = 0; i < 4; ++i) {
                for (auto j = 0; j < 4; ++j) {
                    DecimalType sum = 0.0f;
                    for (auto k = 0; k < 4; ++k) {
                        sum += a.m[i][k] * b.m[k][j];
                    }  
                    m.m[i][j] = sum;
                }
            }

            return m;
        }

        template<typename DecimalType>
        mat4x4_t<DecimalType> Translate(const vec3_t<DecimalType>& p) {
            mat4x4_t<DecimalType> m = mat4x4_t<DecimalType>::Identity();

            m.m[0][3] = p.x;
            m.m[1][3] = p.y;
            m.m[2][3] = p.z;

            return m;
        }

        template<typename DecimalType>
        mat4x4_t<DecimalType> RotateX(const DecimalType& xAngle) {
            mat4x4_t<DecimalType> m = mat4x4_t<DecimalType>::Identity();

            DecimalType c = cosf(xAngle);
            DecimalType s = sinf(xAngle);

            m.m[1][1] = c;
            m.m[1][2] = -s;
            m.m[2][1] = s;
            m.m[2][2] = c;

            return m;
        }

        template<typename DecimalType>
        mat4x4_t<DecimalType> RotateY(const DecimalType& yAngle) {
            mat4x4_t<DecimalType> m = mat4x4_t<DecimalType>::Identity();

            DecimalType c = cosf(yAngle);
            DecimalType s = sinf(yAngle);

            m.m[0][0] = c;
            m.m[0][2] = s;
            m.m[2][0] = -s;
            m.m[2][2] = c;

            return m;
        }

        template<typename DecimalType>
        mat4x4_t<DecimalType> RotateZ(const DecimalType& zAngle) {
            mat4x4_t<DecimalType> m = mat4x4_t<DecimalType>::Identity();

            DecimalType c = cosf(zAngle);
            DecimalType s = sinf(zAngle);

            m.m[0][0] = c;
            m.m[0][1] = -s;
            m.m[1][0] = s;
            m.m[1][1] = c;

            return m;
        }

        template<typename DecimalType>
        mat4x4_t<DecimalType> PerspectiveFOV(DecimalType fovRad, DecimalType aspect, DecimalType near, DecimalType far) {
            mat4x4_t<DecimalType> m = {};

            DecimalType g = 1.0f / tanf(fovRad * 0.5f);

            auto nmf = near - far;
            m.m[0][0] = g / aspect;
            m.m[1][1] = g;
            m.m[2][2] = far / nmf;
            m.m[2][3] = (near * far) / nmf;
            m.m[3][2] = -1.0f;

            return m;
        }

    }

}