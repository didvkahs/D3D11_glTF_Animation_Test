#pragma once

#include<string>


// - Enums - 

enum class eKind
{
	NON = -1,
	SCALAR = 1,
	VEC2 = 2,
	VEC3 = 3,
	VEC4 = 4,
	MAT3 = 9,
	MAT4 = 16
};

enum class eType
{
	BYTE = 5120,
	UBYTE = 5121,
	SHORT = 5122,
	USHOT = 5123,
	UINT = 5125,
	FLOAT = 5126
};

enum class eTarget
{
	VERTEX_ATTRIBUTES = 34962,
	INDEX_ATTRIBUTES = 34963
};

enum class eMagFilter
{
	NEAREST_NEIGHBOR_FILTERING = 9728,
	LINEAR_FILTERING = 9729
};

enum class eMinFilter
{
	GL_NEAREST = 9728,
	GL_LINEAR = 9729,
	GL_NEAREST_MIPMAP_NEAREST = 9984,
	GL_LINEAR_MIPMAP_LINEAR = 9987
};

enum class eWrap
{
	GL_CLAMP_TO_EDGE = 33071,
	GL_REPEAT = 10497,
	GL_MIRRORED_REPEAT = 33648
};



// - Math Types -

typedef float Mat3_t[9];
typedef float Mat4_t[16];

struct Vec4_t
{
	float x = 0, y = 0, z = 0, w = 0;
};

struct Vec2_t
{
	float x = 0, y = 0;
};

struct UVec4_t
{
	uint32_t x = 0, y = 0, z = 0, w = 0;
};



// - GLTF FORMAT -

struct Buffers
{
	std::string binFileName;
	int   byteLength = -1;
};

struct Meshes
{
	long long joint_idx = -1;
	long long position_idx = -1;
	long long normal_idx = -1;
	long long texCoord_idx = -1;
	long long indices_idx = -1;
	long long material_idx = -1;
};

struct GltfObject_Desc
{
	Meshes* meshList = nullptr;
	Buffers* bufferList = nullptr;
};



// - Parsed Data -

struct Vertex
{
	Vec4_t position;
	Vec4_t normal;
	Vec2_t texCoord;
};

struct Samplers
{
	int magFilter = -1;
	int minFilter = -1;
	int wrapS = -1;
	int wrapT = -1;
};

struct Textures
{
	std::string filePath;
	long long sampler = -1;
};

struct Materials
{
	float metallic = -1;
	float roughness = -1;
	long long baseTexture = -1;
};

struct CharactorMesh_Desc
{
	long long vertexCount = 0;
	long long indexCount = 0;
	long long jointCount = 0;
	long long uvCount = 0;

	std::string filePath;
	float metallic = -1;
	float roughness = -1;
	Samplers sampler;

	Vertex*	  vertices  = nullptr;
	uint32_t* indices   = nullptr;
	UVec4_t*  joints    = nullptr;
	Vec2_t*   uvCoord	= nullptr;
};

struct Nodes_Desc
{
	int* children = nullptr;
	int childrenSize = -1;

	bool hasMat = false;
	Mat4_t mat;

	int mesh = -1;
	int skin = -1;

	bool hasRotation = false;
	Vec4_t rotation;
	bool hasScale = false;
	Vec4_t scale;
	bool hasTranslation = false;
	Vec4_t translation;
};



class Parser
{
public:

	bool ParseGLTF(const char* gltf_file);
	void Release(void);

	int GetRootNodeIdx(void) const;

	CharactorMesh_Desc* GetMeshDesc(void) const;
	long long GetMeshCount(void) const;

	Nodes_Desc* GetNodeDesc(void) const;
	long long GetNodeCount(void) const;

private:
	char* binBuffer = nullptr;
	GltfObject_Desc gltfDesc;

	int root_idx = -1;

	CharactorMesh_Desc* ch_list = nullptr;
	long long ch_listSize = 0;

	Nodes_Desc* nd_list = nullptr;
	long long nd_listSize = 0;

};