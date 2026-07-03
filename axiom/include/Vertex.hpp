//
// Created by Liam on 21/06/2026.
//

#pragma once
#include "Debug.hpp"
#include "Maths.hpp"
#include "STL.hpp"
#include "slang-rhi.h"

namespace axm {

    namespace vertex {

        struct Description {
            rhi::VertexStreamDesc m_StreamDesc;
            Vector<rhi::InputElementDesc> m_InputElementDesc;
        };

        struct PosNormalUV {
            vec3 m_Pos;
            vec3 m_Normal;
            vec2 m_UV;

            static rhi::ComPtr<rhi::IInputLayout> GetInputLayout(rhi::IDevice *device) {
                using namespace rhi;
                VertexStreamDesc vertexStreams[] = {
                        {sizeof(PosNormalUV), InputSlotClass::PerVertex, 0},
                };
                InputElementDesc inputElements[] = {
                        {"POSITION", 0, Format::RGB32Float, offsetof(PosNormalUV, m_Pos), 0},
                        {"NORMAL", 0, Format::RGB32Float, offsetof(PosNormalUV, m_Normal), 0},
                        {"UV", 0, Format::RGB32Float, offsetof(PosNormalUV, m_UV), 0},
                };
                InputLayoutDesc inputLayoutDesc = {};
                inputLayoutDesc.inputElements = inputElements;
                inputLayoutDesc.inputElementCount = sizeof(inputElements) / sizeof(inputElements[0]);
                inputLayoutDesc.vertexStreams = vertexStreams;
                inputLayoutDesc.vertexStreamCount = sizeof(vertexStreams) / sizeof(vertexStreams[0]);
                ComPtr<IInputLayout> inputLayout;
                if (SLANG_FAILED(device->createInputLayout(inputLayoutDesc, inputLayout.writeRef()))) {
                    AXM_LOG("Failed to create input layout");
                    return nullptr;
                }

                return inputLayout;
            }
        };


    }; // namespace vertex
}