#pragma once

#include<fstream>
#include<string>


// - Consts - 

#define KIND_NON	(-1)
#define KIND_SCALAR (1)
#define KIND_VEC2	(2)
#define KIND_VEC3	(3)
#define KIND_VEC4	(4)
#define KIND_MAT3	(9)
#define KIND_MAT4	(16)

#define TYPE_BYTE	(5120)
#define TYPE_UBYTE	(5121)
#define TYPE_SHORT	(5122)
#define TYPE_USHORT	(5123)
#define TYPE_UINT	(5125)
#define TYPE_FLOAT	(5126)

#define MODE_POINTLIST		(0)
#define MODE_LINELIST		(1)
#define MODE_LINESTRIP		(3)
#define MODE_TRIANGLELIST	(4)
#define MODE_TRIANGLESTRIP	(5)

#define TARGET_VERTEX_ATTRIBUTES (34962)
#define TARGET_INDEX_ATTRIBUTES  (34963)


// - Math Types -

typedef float Mat3_t[3][3];
typedef float Mat4_t[4][4];

struct Vec4_t
{
	float x = 0, y = 0, z = 0, w = 0;
};

struct Vec2_t
{
	float x = 0, y = 0;
};


// - GLTF FORMAT -

struct Buffers
{
	std::string binFileName;
	int   byteLength = -1;
};

struct Meshes
{
	int position_idx = -1;
	int normal_idx = -1;
	int texCoord_idx = -1;
	int indices_idx = -1;
};

struct GltfObject_Desc
{
	Meshes* meshList = nullptr;
	Buffers* bufferList = nullptr;
};



// - Vertex & Index Buffer - 

struct Vertex
{
	Vec4_t position;
	Vec4_t normal;
	Vec2_t texCoord;
};


class Loader
{
public:

	bool ParseGLTF(const char* gltf_file);
	void Release(void);

	long long GetIndexCount(void) const;
	long long GetVertexCount(void) const;
	Vertex* GetVertices(void) const;
	uint32_t* GetIndices(void) const;

private:

	GltfObject_Desc gltfDesc;

	uint32_t* indices = nullptr;
	Vertex* vertices = nullptr;
	
	long long indexCount = 0;
	long long vertexCount = 0;

	char* binBuffer = nullptr;

};