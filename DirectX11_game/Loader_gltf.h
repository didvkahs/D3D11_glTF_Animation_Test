#pragma once

#include"ILoader.h"
#include"Parser_gltf.h"

struct DX_Enums_s;

struct DX_Node_s
{
	DirectX::XMMATRIX normalMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldTransform = DirectX::XMMatrixIdentity();

	int mesh_idx = -1;
	int skin_idx = -1;
	int parent_idx = -1;
	int currnet_idx = -1;
};

struct DX_Texture_s
{
	std::string filePath;
	DX_Enums_s* enums = nullptr;
};

class Loader_gltf : public ILoader
{
public:

	Loader_gltf(const char* filePath);
	~Loader_gltf();

	bool ParseFile(void) override;
	
	const DX_Texture_s* GetTextures(void) const;
	size_t GetTextureCount(void) const;

private:

	void CalNode(const DirectX::XMMATRIX parentMat, const int p_idx, const int nd_idx, Nodes_Desc* list);

private:

	Parser m_parser;
	
	eastl::vector<DX_Node_s> m_nodes;
	eastl::vector<DX_Texture_s> m_textures;

	int m_rootMesh_idx = -1;
};