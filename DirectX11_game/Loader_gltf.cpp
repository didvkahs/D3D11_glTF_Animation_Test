#include"Loader_gltf.h"

#include "Utile.h"

using namespace DirectX;

struct DX_Enums_s
{
	D3D11_FILTER filter;
	D3D11_TEXTURE_ADDRESS_MODE u;
	D3D11_TEXTURE_ADDRESS_MODE v;
};

Loader_gltf::Loader_gltf(const char* filePath) : ILoader(filePath) 
{
}

Loader_gltf::~Loader_gltf(void)
{
	for (size_t i = 0; i < m_textures.size(); ++i)
	{
		delete m_textures[i].enums;
	}
}


bool Loader_gltf::ParseFile(void)
{
	m_parser.ParseGLTF(m_filePath);

	// - Get GLTF Datas - 
	CharactorMesh_Desc* ch_list = m_parser.GetMeshDesc();
	long long ch_listSize = m_parser.GetMeshCount();

	Nodes_Desc* nd_list = m_parser.GetNodeDesc();
	long long nd_listSize = m_parser.GetNodeCount();

	if (ch_list == nullptr)
	{
		return false;
	}

	if (nd_list == nullptr)
	{
		return false;
	}

	const size_t meshLength = ch_listSize;
	m_textures.resize(ch_listSize);

	{ // - Store ch_list into meshes - 
		m_meshList.resize(ch_listSize);

		size_t ver_count;
		size_t ind_count;
		size_t joi_count;

		for (size_t i = 0; i < meshLength; ++i)
		{
			ver_count = ch_list[i].vertexCount;
			ind_count = ch_list[i].indexCount;
			joi_count = ch_list[i].jointCount;

			m_meshList[i].vertices.resize(ver_count);
			m_meshList[i].indices.resize(ind_count);

			auto& vertices = m_meshList[i].vertices;
			// - Get Vertices -
			for (size_t j = 0; j < ver_count; ++j)
			{
				const auto& src_nor = ch_list[i].vertices[j].normal;
				const auto& src_pos = ch_list[i].vertices[j].position;
				const auto& src_tex = ch_list[i].vertices[j].texCoord;

				vertices[j].normal = XMFLOAT4(src_nor.x, src_nor.y, src_nor.z, src_nor.w);
				vertices[j].position = XMFLOAT4(src_pos.x, src_pos.y, src_pos.z, src_pos.w);
				vertices[j].texCoord = XMFLOAT2(src_tex.x, src_tex.y);
			}


			// - Get Indices -
			for (size_t j = 0; j < ind_count; ++j)
			{
				m_meshList[i].indices[j] = ch_list[i].indices[j];
			}


			// - Get Textures -

			m_textures[i].filePath = ch_list[i].filePath;
			m_textures[i].enums = new DX_Enums_s;
			
			int magVal = ch_list[i].sampler.magFilter;
			int minVal = ch_list[i].sampler.minFilter;

			eMagFilter mag = static_cast<eMagFilter>(ch_list[i].sampler.magFilter);
			eMinFilter min = static_cast<eMinFilter>(ch_list[i].sampler.minFilter);

			if (
				magVal == static_cast<int>(eMagFilter::NEAREST_NEIGHBOR_FILTERING)
				&& minVal == static_cast<int>(eMinFilter::GL_NEAREST)
			)
			{
				m_textures[i].enums->filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			}
			else if (
				magVal == static_cast<int>(eMagFilter::NEAREST_NEIGHBOR_FILTERING)
				&& minVal == static_cast<int>(eMinFilter::GL_NEAREST)
			)
			{
				m_textures[i].enums->filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			}
			else if (
				magVal == static_cast<int>(eMagFilter::LINEAR_FILTERING)
				&& minVal == static_cast<int>(eMinFilter::GL_LINEAR_MIPMAP_LINEAR)
				)
			{
				m_textures[i].enums->filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			}
			else if (
				magVal == static_cast<int>(eMagFilter::NEAREST_NEIGHBOR_FILTERING)
				&& minVal == static_cast<int>(eMinFilter::GL_NEAREST_MIPMAP_NEAREST)
				)
			{
				m_textures[i].enums->filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

			}
			
			int sVal = ch_list[i].sampler.wrapS;
			int tVal = ch_list[i].sampler.wrapT;

			switch (sVal)
			{
			case static_cast<int>(eWrap::GL_REPEAT):
				m_textures[i].enums->u = D3D11_TEXTURE_ADDRESS_WRAP;
				break;
			case static_cast<int>(eWrap::GL_CLAMP_TO_EDGE):
				m_textures[i].enums->u = D3D11_TEXTURE_ADDRESS_CLAMP;
				break;
			case static_cast<int>(eWrap::GL_MIRRORED_REPEAT):
				m_textures[i].enums->u = D3D11_TEXTURE_ADDRESS_MIRROR;
				break;
			}

			switch (tVal)
			{
			case static_cast<int>(eWrap::GL_REPEAT):
				m_textures[i].enums->u = D3D11_TEXTURE_ADDRESS_WRAP;
				break;
			case static_cast<int>(eWrap::GL_CLAMP_TO_EDGE):
				m_textures[i].enums->u = D3D11_TEXTURE_ADDRESS_CLAMP;
				break;
			case static_cast<int>(eWrap::GL_MIRRORED_REPEAT):
				m_textures[i].enums->u = D3D11_TEXTURE_ADDRESS_MIRROR;
				break;
			}
		}
	}


	{ // - Cal Nodes -

		m_nodes.resize(nd_listSize);

		int root = m_parser.GetRootNodeIdx();
		int childrenSize = nd_list[root].childrenSize;

		XMMATRIX localMat;
		std::memcpy(&localMat, nd_list[root].mat, sizeof(float) * 16);
		localMat = XMMatrixTranspose(localMat);

		m_nodes[root].currnet_idx = root;
		m_nodes[root].mesh_idx = nd_list[root].mesh;
		m_nodes[root].skin_idx = nd_list[root].skin;
		m_nodes[root].worldTransform = localMat;

		if (childrenSize)
		{
			for (int i = 0; i < childrenSize; ++i)
			{
				CalNode(localMat, root, nd_list[root].children[i], nd_list);
			}
		}
	}

	for (size_t i = 0; i < m_nodes.size(); ++i)
	{
		if (m_nodes[i].mesh_idx != -1)
		{
			m_rootMesh_idx = static_cast<int>(i);
			goto escape;
		}
	}

escape:

	m_parser.Release();

	// - Set m_meshList boneTransformation -

	if (m_rootMesh_idx == -1)
	{
		return false;
	}

	for (size_t i = 0; i < meshLength; ++i)
	{
		m_meshList[i].boneTransform = m_nodes[m_rootMesh_idx + i].worldTransform;
	}

	return true;
}

void Loader_gltf::CalNode(const XMMATRIX parentMat, const int p_idx, const int nd_idx, Nodes_Desc* list)
{
	Nodes_Desc current = list[nd_idx];
	XMMATRIX localMat = XMMatrixIdentity();

	if (current.hasMat == true)
	{
		std::memcpy(&localMat, current.mat, sizeof(float) * 16);
		localMat = XMMatrixTranspose(localMat);
	}
	else if (current.hasRotation && current.hasScale && current.hasTranslation)
	{
		XMVECTOR t, r, s;

		t = XMVectorSet(current.translation.x, current.translation.y, current.translation.z, current.translation.w);
		r = XMVectorSet(current.rotation.x, current.rotation.y, current.rotation.z, current.rotation.w);
		s = XMVectorSet(current.scale.x, current.scale.y, current.scale.z, current.scale.w);

		XMMATRIX mt, mr, ms;

		mt = XMMatrixTranslationFromVector(t);
		mr = XMMatrixRotationQuaternion(r);
		ms = XMMatrixScalingFromVector(s);

		localMat = ms * mr * mt;
	}

	m_nodes[nd_idx].currnet_idx = nd_idx;
	m_nodes[nd_idx].parent_idx = p_idx;
	m_nodes[nd_idx].mesh_idx = list[nd_idx].mesh;
	m_nodes[nd_idx].skin_idx = list[nd_idx].skin;

	m_nodes[nd_idx].worldTransform = XMMatrixMultiply(parentMat, localMat);

	int ch_size = list[nd_idx].childrenSize;
	if (ch_size)
	{
		for (int i = 0; i < ch_size; ++i)
		{
			CalNode(m_nodes[nd_idx].worldTransform, nd_idx, list[nd_idx].children[i], list);
		}
	}
}

const DX_Texture_s* Loader_gltf::GetTextures(void) const
{
	assert(m_textures.size() != 0 && "Error : textures is null");
	
	return m_textures.data();
}

size_t Loader_gltf::GetTextureCount(void) const
{
	return m_textures.size();
}