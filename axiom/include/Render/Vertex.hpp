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


            template <size_t ElemCount>
            Layout(size_t dataElemSize,
                   Array<rhi::InputElementDesc, ElemCount> inputElements,
                   u32 instanceDataStepRate = 0) : m_VertexStream({ }) {

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
            explicit Layout(Array<rhi::InputElementDesc, ElementCount> inputElements, u32 instanceDataStepRate = 0) :
                Layout(sizeof(VertexElementType), inputElements, instanceDataStepRate) { }

            NO_DISCARD size_t GetElementSize() const {
                size_t size = 0;

                for (auto& e: m_InputElements) {
                    size += e.offset;
                }

                return size;
            }

            NO_DISCARD rhi::ComPtr<rhi::IInputLayout> GetDeviceLayout(rhi::IDevice* device) const {
                using namespace rhi;

                return { };
            }
        };


        struct PosNormalUV
        {
            vec3 m_Pos;
            vec3 m_Normal;
            vec2 m_UV;

            static rhi::ComPtr<rhi::IInputLayout> GetInputLayout(rhi::IDevice* device) {
                using namespace rhi;
                VertexStreamDesc vertexStreams[] = {
                    { sizeof(PosNormalUV), InputSlotClass::PerVertex, 0 },
                };
                InputElementDesc inputElements[] = {
                    { "POSITION", 0, Format::RGB32Float, offsetof(PosNormalUV, m_Pos), 0 },
                    { "NORMAL", 0, Format::RGB32Float, offsetof(PosNormalUV, m_Normal), 0 },
                    { "UV", 0, Format::RGB32Float, offsetof(PosNormalUV, m_UV), 0 },
                };
                InputLayoutDesc inputLayoutDesc = { };
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
} // namespace axm
