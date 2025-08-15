#pragma once

#include<EASTL/vector.h>
#include<DirectXMath.h>


struct DX_Vertex_s
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT2 texCoord;
};

struct DX_MeshData_s
{
	DirectX::XMMATRIX boneTransform = DirectX::XMMatrixIdentity();
	eastl::vector<DX_Vertex_s> vertices;
	eastl::vector<uint32_t> indices;
};


class ILoader
{
public:

	ILoader(void);
	ILoader(const char* filePath);

	virtual bool ParseFile(void) = 0;

	const DX_Vertex_s* GetVertices(const size_t v_idx);
	const size_t GetVertexLength(const size_t v_idx);
	const uint32_t* GetIndices(const size_t i_idx);
	const size_t GetIndexLength(const size_t i_idx);
	const size_t GetMeshLength(void) const;
	DirectX::XMMATRIX GetTransform(const size_t m_idx) const;

protected:

	const char* m_filePath = nullptr;
	eastl::vector<DX_MeshData_s> m_meshList;

};