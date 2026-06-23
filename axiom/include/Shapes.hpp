#pragma once
#include "Prim.hpp"

namespace axm::shapes {

    struct Description {
        f32*    m_VertexBuffer;
        size_t  m_BufferLength;
        i32*    m_IndexBuffer;
        i32     m_NumIndices;

    };

    Description GetCubeShape();
}