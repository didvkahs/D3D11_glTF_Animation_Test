#include"ILoader.h"

#include<assert.h>


ILoader::ILoader(void)
    : m_filePath(nullptr)
{
}

ILoader::ILoader(const char* filePath)
    : m_filePath(filePath)
{
}

const DX_Vertex_s* ILoader::GetVertices(const size_t v_idx)
{
    assert(m_meshList.size() != 0 && "Error: meshList is null");
    assert(v_idx >= 0 && v_idx < m_meshList.size() && "Error: invalid index");

    const DX_MeshData_s* mesh = m_meshList.data() + v_idx;
    assert(mesh != nullptr && "Error: mesh is null");
    assert(mesh->vertices.size()> 0 && "Error: invalid vertex length");
    assert(mesh->vertices.data() != nullptr && "Error: vertices array is null");

    return mesh->vertices.data();
}

const size_t ILoader::GetVertexLength(const size_t v_idx)
{
    assert(m_meshList.size() != 0 && "Error: meshList is null");
    assert(v_idx >= 0 && v_idx < m_meshList.size() && "Error: invalid index");

    const DX_MeshData_s* mesh = m_meshList.data() + v_idx;
    assert(mesh != nullptr && "Error: mesh is null");
    assert(mesh->vertices.size()> 0 && "Error: invalid vertex length");

    return mesh->vertices.size();
}

const uint32_t* ILoader::GetIndices(const size_t i_idx)
{
    assert(m_meshList.size() != 0 && "Error: meshList is null");
    assert(i_idx >= 0 && i_idx < m_meshList.size() && "Error: invalid index");

    const DX_MeshData_s* mesh = m_meshList.data() + i_idx;
    assert(mesh != nullptr && "Error: mesh is null");
    assert(mesh->indices.size()> 0 && "Error: invalid index length");
    assert(mesh->indices.data() != nullptr && "Error: indices array is null");

    return mesh->indices.data();
}

const size_t ILoader::GetIndexLength(const size_t i_idx)
{
    assert(m_meshList.size() != 0 && "Error: meshList is null");
    assert(i_idx >= 0 && i_idx < m_meshList.size()  && "Error: invalid index");

    const DX_MeshData_s* mesh = m_meshList.data() + i_idx;
    assert(mesh != nullptr && "Error: mesh is null");
    assert(mesh->indices.size() > 0 && "Error: invalid vertex length");

    return mesh->indices.size();
}

const size_t ILoader::GetMeshLength(void) const
{
    return m_meshList.size();
}

DirectX::XMMATRIX ILoader::GetTransform(const size_t m_idx) const
{
    assert(m_meshList.size() != 0 && "Error: meshList is null");
    assert(m_idx >= 0 && m_idx < m_meshList.size() && "Error: invalid index");

    const DX_MeshData_s* mesh = m_meshList.data() + m_idx;
    assert(mesh != nullptr && "Error: mesh is null");

    return mesh->boneTransform;
}