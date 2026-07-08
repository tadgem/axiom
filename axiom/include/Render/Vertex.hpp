//
// Created by Liam on 21/06/2026.
//

#pragma once
#include "Core/Debug.hpp"
#include "Core/Maths.hpp"
#include "Core/STL.hpp"
#include "slang-rhi.h"

namespace axm {

    namespace vertex {

        class Layout
        {
        public:
            Vector<rhi::InputElementDesc> m_InputElements;
            rhi::VertexStreamDesc m_VertexStream;
            rhi::ComPtr<rhi::IInputLayout> m_DeviceInputLayout;

            Layout() = default;

            template <size_t ElemCount>
            Layout(size_t dataElemSize,
                   Array<rhi::InputElementDesc, ElemCount> inputElements,
                   u32 instanceDataStepRate = 0) : m_VertexStream({ }), m_DeviceInputLayout(nullptr) {

                m_InputElements.resize(ElemCount);
                for (auto i = 0; i < ElemCount; i++) {
                    m_InputElements[i] = inputElements[i];
                }

                m_VertexStream.stride = dataElemSize;
                m_VertexStream.slotClass
                        = instanceDataStepRate > 0 ? rhi::InputSlotClass::PerInstance : rhi::InputSlotClass::PerVertex;
                m_VertexStream.instanceDataStepRate = instanceDataStepRate;
            }

            template <typename VertexElementType, size_t ElementCount>
            static Layout BuildLayout(Array<rhi::InputElementDesc, ElementCount> inputElements,
                                      u32 instanceDataStepRate = 0) {

                return Layout(sizeof(VertexElementType), inputElements, instanceDataStepRate);
            }

            NO_DISCARD size_t GetElementSize() const {
                size_t size = 0;

                for (auto& e: m_InputElements) {
                    size += e.offset;
                }

                return size;
            }

            void BuildDeviceLayout(rhi::IDevice* device) {
                using namespace rhi;

                VertexStreamDesc vertexStreams[] = { m_VertexStream };

                InputLayoutDesc inputLayoutDesc = { };
                inputLayoutDesc.inputElements = m_InputElements.data();
                inputLayoutDesc.inputElementCount = m_InputElements.size();
                inputLayoutDesc.vertexStreams = vertexStreams;
                // TODO: will we need more than one stream of vertices?
                inputLayoutDesc.vertexStreamCount = 1;

                if (SLANG_FAILED(device->createInputLayout(inputLayoutDesc, m_DeviceInputLayout.writeRef()))) {
                    AXM_LOG("Failed to create input layout");
                }
            }
        };


        struct PosNormalUV
        {
            vec3 m_Pos;
            vec3 m_Normal;
            vec2 m_UV;

            static Layout GetInputLayout() {
                using namespace rhi;
                return Layout::BuildLayout<PosNormalUV, 3>({
                        InputElementDesc { "Position", 0, Format::RGB32Float, offsetof(PosNormalUV, m_Pos), 0 },
                        InputElementDesc { "Normal", 1, Format::RGB32Float, offsetof(PosNormalUV, m_Normal), 0 },
                        InputElementDesc { "UV", 2, Format::RG32Float, offsetof(PosNormalUV, m_UV), 0 },
                });
            }
        };
    }; // namespace vertex
} // namespace axm
