#pragma once
#include "Core/STL.hpp"
#include "Render/Viewport.hpp"
#include "vku/vku.h"

namespace axm
{

struct CPUMesh
{
	String m_Name;
	void*  m_CPUMemory;
	CPUMesh(String name, void* data) : m_Name(std::move(name)), m_CPUMemory(data) {};
};

struct GPUMesh
{
	static constexpr size_t MAX_BONES_PER_VERTEX = 4;
	struct VertexBones
	{
		u16 m_BoneIndex[MAX_BONES_PER_VERTEX];
		f16 m_BoneWeight[MAX_BONES_PER_VERTEX];
	};

	vku::Buffer m_VertexBuffer;
	vku::Buffer m_IndexBuffer;
	u64         m_IndexCount = 0;
	bool        m_Valid      = false;

	void        Free(vku::VkState& vk)
	{
		m_VertexBuffer.Free(vk);
		m_IndexBuffer.Free(vk);
		m_Valid = false;
	}
};

namespace meshes
{
	GPUMesh CreateMeshFromData(vku::VkState& vk,
	                           const void*   vertexData,
	                           u64           vertexDataSize,
	                           const u32*    indexData,
	                           u64           numIndices,
	                           const char*   label = "AnonMesh");

	void    Bind(const GPUMesh& mesh, VkCommandBuffer cmd);
	void    DrawMesh(const Viewport& viewPort, const GPUMesh& mesh, VkCommandBuffer cmd);
}  // namespace meshes
}  // namespace axm
