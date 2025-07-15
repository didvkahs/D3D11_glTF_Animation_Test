#include"glTF_Loader.h"

#include<json.hpp>
#include<iostream>

typedef nlohmann::json json;


void Loader::Release(void)
{
	if (indices) { delete[] indices; }
	if (gltfDesc.bufferList) { delete[] gltfDesc.bufferList; }
	if (gltfDesc.meshList) { delete[] gltfDesc.meshList; }

	if (vertices) { delete[] vertices; }

	if (binBuffer != nullptr) { delete[] binBuffer; }
}

bool Loader::ParseGLTF(const char* gltf_file)
{
	// - Open file & parse josn -

	std::fstream file(gltf_file);

	if (!file.is_open())
	{
		std::cerr << "Error : (GLTF) Failed to open GLTF file : " << gltf_file << " \n";
		return false;
	}

	json j;

	try {
		file >> j;
	}
	catch (const json::parse_error& e) {
		file.close();
		std::cerr << "JSON parse error: " << e.what() << std::endl;
		return false;
	}

	file.close();


	// - gltf Desc -


	{
		if (!j.contains("buffers"))
		{
			std::cerr << "Error : (GLTF-BUFFERS) File doesn't contains buffers\n";
			return false;
		}

		const auto& buffer = j["buffers"];
		size_t bufferSize = buffer.size();

		Buffers* bfList = new Buffers[bufferSize];

		for (int i = 0; i < bufferSize; ++i)
		{
			bfList[i].binFileName = buffer[i]["uri"].get<std::string>();
			bfList[i].byteLength = buffer[i]["byteLength"].get<int>();
		}

		std::ifstream bFile(bfList->binFileName, std::ios::binary);
		if (!bFile.is_open())
		{
			std::cerr << "Error : (GLTF-BINBUFFER) File doesn't contains binBuffer\n";
			return false;
		}

		binBuffer = new char[bfList->byteLength];
		bFile.read(binBuffer, bfList->byteLength);
		bFile.close();

		gltfDesc.bufferList = bfList;
	}

	size_t meshSize = 0;
	{
		if (!j.contains("meshes"))
		{
			std::cerr << "Error : (GLTF-MESHES) File doesn't contians meshes\n";
			return false;
		}

		const auto& mesh = j["meshes"];
		meshSize = mesh.size();
		Meshes* mhList = new Meshes[meshSize];

		for (int i = 0; i < meshSize; ++i)
		{
			auto& primitive = mesh[i]["primitives"][0];
			auto& attributes = primitive["attributes"];

			mhList[i].indices_idx = primitive["indices"].get<int>();
			mhList[i].normal_idx = attributes["NORMAL"].get<int>();
			mhList[i].position_idx = attributes["POSITION"].get<int>();
			mhList[i].texCoord_idx = attributes["TEXCOORD_0"].get<int>();
		}

		gltfDesc.meshList = mhList;
	}


	// - Get Vertex & index Size
	const auto& accessor = j["accessors"];

	{
		long long posSize = 0;
		long long norSize = 0;
		long long texSize = 0;
		long long indSize = 0;


		for (int i = 0; i < meshSize; ++i)
		{
			{ // Position

				int ac_idx = gltfDesc.meshList[i].position_idx;
				int ac_count = accessor[ac_idx]["count"].get<int>();
				posSize += ac_count;

			}

			{ // Indices
				int ac_idx = gltfDesc.meshList[i].indices_idx;
				int ac_count = accessor[ac_idx]["count"].get<int>();
				indSize += ac_count;
			}
		}

		vertexCount = posSize;
		indexCount = indSize;
		vertices = new Vertex[posSize];
		indices = new uint32_t[indSize];
	}


	// - Extract Vertex & Index -
	const auto& bfView = j["bufferViews"];

	long long pos_idx = 0;
	long long nor_idx = 0;
	long long tex_idx = 0;
	long long ind_idx = 0;

	for (int i = 0; i < meshSize; ++i)
	{
		{ // Position
			int ac_idx = gltfDesc.meshList[i].position_idx;
			int bv_idx = accessor[ac_idx]["bufferView"].get<int>();

			int ac_count = accessor[ac_idx]["count"].get<int>();
			int bv_offset = bfView[bv_idx]["byteOffset"].get<int>();

			int ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<int>();
			}

			int stride = 0;

			if (bfView[bv_idx].contains("btyeOffset"))
			{
				stride = bfView[bv_idx]["byteOffset"].get<int>();
			}
			else
			{
				int ac_kind = 1;
				if (accessor[ac_idx].contains("min"))
				{
					ac_kind = accessor[ac_idx]["min"].size();
				}
				stride = 4 * ac_kind;
			}

			int initAddress = ac_offset + bv_offset;

			for (int j = 0; j < ac_count; ++j)
			{
				float* pos = reinterpret_cast<float*>(binBuffer + initAddress + j * stride);

				vertices[pos_idx].position.x = pos[0];
				vertices[pos_idx].position.y = pos[1];
				vertices[pos_idx].position.z = pos[2];
				vertices[pos_idx].position.w = 1;

				++pos_idx;
			}
		}


		{ // Normal
			int ac_idx = gltfDesc.meshList[i].normal_idx;
			int bv_idx = accessor[ac_idx]["bufferView"].get<int>();

			int ac_count = accessor[ac_idx]["count"].get<int>();
			int bv_offset = bfView[bv_idx]["byteOffset"].get<int>();

			int ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<int>();
			}

			int stride = 0;
			if (bfView[bv_idx].contains("btyeOffset"))
			{
				stride = bfView[bv_idx]["byteOffset"].get<int>();
			}
			else
			{
				int ac_kind = 1;
				if (accessor[ac_idx].contains("min"))
				{
					ac_kind = accessor[ac_idx]["min"].size();
				}
				stride = 4 * ac_kind;
			}

			int initAddress = ac_offset + bv_offset;

			for (int j = 0; j < ac_count; ++j)
			{
				float* pos = reinterpret_cast<float*>(binBuffer + initAddress + j * stride);

				vertices[nor_idx].normal.x = pos[0];
				vertices[nor_idx].normal.y = pos[1];
				vertices[nor_idx].normal.z = pos[2];
				vertices[nor_idx].normal.w = 0;

				++nor_idx;
			}
		}

		{ // TexCoord
			int ac_idx = gltfDesc.meshList[i].texCoord_idx;
			int bv_idx = accessor[ac_idx]["bufferView"].get<int>();

			int ac_count = accessor[ac_idx]["count"].get<int>();
			int bv_offset = bfView[bv_idx]["byteOffset"].get<int>();

			int ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<int>();
			}

			int stride = 0;

			if (bfView[bv_idx].contains("btyeOffset"))
			{
				stride = bfView[bv_idx]["byteOffset"].get<int>();
			}
			else
			{
				int ac_kind = 1;
				if (accessor[ac_idx].contains("min"))
				{
					ac_kind = accessor[ac_idx]["min"].size();
				}
				stride = 4 * ac_kind;
			}

			int initAddress = ac_offset + bv_offset;

			for (int j = 0; j < ac_count; ++j)
			{
				float* pos = reinterpret_cast<float*>(binBuffer + initAddress + j * stride);

				vertices[tex_idx].texCoord.x = pos[0];
				vertices[tex_idx].texCoord.y = pos[1];
				++tex_idx;
			}
		}


		{// indices
			int ac_idx = gltfDesc.meshList[i].indices_idx;
			int bv_idx = accessor[ac_idx]["bufferView"].get<int>();

			int ac_count = accessor[ac_idx]["count"].get<int>();
			int bv_offset = bfView[bv_idx]["byteOffset"].get<int>();

			int ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<int>();
			}

			int stride = 0;

			if (bfView[bv_idx].contains("btyeOffset"))
			{
				stride = bfView[bv_idx]["byteOffset"].get<int>();
			}
			else
			{
				int ac_kind = 1;
				if (accessor[ac_idx].contains("min"))
				{
					ac_kind = accessor[ac_idx]["min"].size();
				}
				stride = 4 * ac_kind;
			}

			int initAddress = ac_offset + bv_offset;

			for (int j = 0; j < ac_count; ++j)
			{
				uint32_t pos = *reinterpret_cast<uint32_t*>(binBuffer + initAddress + j * stride);

				indices[ind_idx] = pos;

				++ind_idx;
			}
		}
	}


	// PrintVertices(vertices);
	//PrintIndices(indices);

	delete[] binBuffer;
	binBuffer = nullptr;

	return true;
}

Vertex* Loader::GetVertices(void) const
{
	return vertices;
}

uint32_t* Loader::GetIndices(void) const
{
	return indices;
}

long long Loader::GetIndexCount(void) const
{
	return indexCount;
}

long long Loader::GetVertexCount(void) const
{
	return vertexCount;
}