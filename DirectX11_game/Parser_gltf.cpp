#include"Parser_gltf.h"

#include<json.hpp>
#include<iostream>
#include<fstream>


typedef nlohmann::json json;

inline int GetStride(const json& bfView, int bv_idx, const json& accessor, int ac_idx, int byte)
{
	if (bfView[bv_idx].contains("byteStride"))
	{
		return bfView[bv_idx]["byteStride"].get<int>();
	}
	else
	{
		int ac_kind = 1;

		if (accessor[ac_idx].contains("type"))
		{
			std::string type = accessor[ac_idx]["type"].get<std::string>();

			if (type == "VEC4") { ac_kind = 4; }
			else if (type == "VEC3") { ac_kind = 3; }
			else if (type == "VEC2") { ac_kind = 2; }
		}

		return byte * ac_kind;
	}
}

void Parser::Release(void)
{
	if (gltfDesc.bufferList != nullptr) { delete[] gltfDesc.bufferList; }
	if (gltfDesc.meshList != nullptr) { delete[] gltfDesc.meshList; }
	if (binBuffer != nullptr) { delete[] binBuffer; }
}

bool Parser::ParseGLTF(const char* gltf_file)
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
		ch_list = new CharactorMesh_Desc[meshSize];
		ch_listSize = meshSize;

		for (int i = 0; i < meshSize; ++i)
		{
			auto& primitive = mesh[i]["primitives"][0];
			auto& attributes = primitive["attributes"];

			mhList[i].indices_idx = primitive["indices"].get<long long>();
			mhList[i].material_idx = primitive["material"].get<long long>();

			mhList[i].normal_idx = attributes["NORMAL"].get<long long>();
			mhList[i].joint_idx = attributes["JOINTS_0"].get<long long>();
			mhList[i].position_idx = attributes["POSITION"].get<long long>();
			mhList[i].texCoord_idx = attributes["TEXCOORD_0"].get<long long>();
		}

		gltfDesc.meshList = mhList;
	}


	const auto& accessor = j["accessors"];
	{
		long long ver_size = 0;
		long long ind_size = 0;
		long long joi_size = 0;
		long long uvc_size = 0;

		for (long long i = 0; i < meshSize; ++i)
		{
			long long v_ac_idx = gltfDesc.meshList[i].position_idx;
			long long i_ac_idx = gltfDesc.meshList[i].indices_idx;
			long long j_ac_idx = gltfDesc.meshList[i].joint_idx;
			long long u_ac_idx = gltfDesc.meshList[i].texCoord_idx;

			ver_size = accessor[v_ac_idx]["count"].get<long long>();
			ind_size = accessor[i_ac_idx]["count"].get<long long>();
			joi_size = accessor[j_ac_idx]["count"].get<long long>();
			uvc_size = accessor[u_ac_idx]["count"].get<long long>();

			ch_list[i].vertexCount = ver_size;
			ch_list[i].indexCount = ind_size;
			ch_list[i].jointCount = joi_size;
			ch_list[i].uvCount = uvc_size;

			ch_list[i].vertices = new Vertex  [ver_size];
			ch_list[i].indices  = new uint32_t[ind_size];
			ch_list[i].joints   = new UVec4_t [joi_size];
			ch_list[i].uvCoord  = new Vec2_t  [uvc_size];
		}
	}

	{ // - Get Root Node - 
		const auto& scene = j["scenes"];
		
		root_idx = scene[0]["nodes"][0].get<int>();
	}

	// - Extract Texture Datas - 

	Textures* tex_list = nullptr;
	long long tex_listSize = -1;

	Samplers* sam_list = nullptr;
	long long sam_listSize = -1;

	Materials* ma_list = nullptr;
	long long  ma_listSize = -1;

	{
		// -- Get image Path --

		if (!j.contains("textures"))
		{
			std::cerr << "json doesn't contains textures\n";
			return false;
		}

		const auto& texture = j["textures"];
		const auto& image = j["images"];

		tex_listSize = texture.size();
		tex_list = new Textures[tex_listSize];

		for (long long i = 0; i < tex_listSize; ++i)
		{
			tex_list[i].filePath = image[i]["uri"].get<std::string>();
			tex_list[i].sampler = texture[i]["sampler"].get<long long>();
		}


		// -- Get Materials --

		if (!j.contains("materials"))
		{
			std::cerr << "json doesn't contains materials\n";
			return false;
		}

		const auto& material = j["materials"];
		
		ma_listSize = material.size();
		ma_list = new Materials[ma_listSize];

		for (long long i = 0; i < ma_listSize; ++i)
		{
			const auto& pbrMetallic = material[i]["pbrMetallicRoughness"];
			const auto& baseTexture = pbrMetallic["baseColorTexture"];


			ma_list[i].baseTexture = baseTexture["index"].get<long long>();
			ma_list[i].metallic = pbrMetallic["metallicFactor"].get<float>();
			ma_list[i].roughness = pbrMetallic["roughnessFactor"].get<float>();
		}

		
		// -- Get Samplers --

		if (!j.contains("samplers"))
		{
			std::cerr << "json doesn't contains samplers\n";
			return false;
		}

		const auto& sampler = j["samplers"];

		sam_listSize = sampler.size();
		sam_list = new Samplers[sam_listSize];

		for (long long i = 0; i < sam_listSize; ++i)
		{
			sam_list[i].magFilter = sampler[i]["magFilter"].get<int>();
			sam_list[i].minFilter = sampler[i]["minFilter"].get<int>();
			sam_list[i].wrapS = sampler[i]["wrapS"].get<int>();
			sam_list[i].wrapT = sampler[i]["wrapT"].get<int>();
		}
	}


	// - Extract Vertex & Index -

	const auto& bfView = j["bufferViews"];

	for (long long i = 0; i < meshSize; ++i)
	{
		{ // - Joints -
			long long ac_idx = gltfDesc.meshList[i].joint_idx;
			long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();

			long long ac_count = accessor[ac_idx]["count"].get<long long>();

			long long bv_offset = bfView[bv_idx]["byteOffset"].get<long long>();
			long long ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<long long>();
			}

			int stride = GetStride(bfView, bv_idx, accessor, ac_idx, sizeof(uint32_t));
			int binOffset = bv_offset + ac_offset;


			for (long long j = 0; j < ac_count; ++j)
			{
				uint16_t* pos = reinterpret_cast<uint16_t*>(binBuffer + binOffset + j * stride);
				ch_list[i].joints[j].x = static_cast<uint32_t>(pos[0]);
				ch_list[i].joints[j].y = static_cast<uint32_t>(pos[1]);;
				ch_list[i].joints[j].z = static_cast<uint32_t>(pos[2]);;
				ch_list[i].joints[j].w = static_cast<uint32_t>(pos[3]);;
			}
		}

		{ //  - Position -
			long long ac_idx = gltfDesc.meshList[i].position_idx;
			long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();

			long long ac_count = accessor[ac_idx]["count"].get<long long>();

			long long bv_offset = bfView[bv_idx]["byteOffset"].get<long long>();
			long long ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<long long>();
			}

			int stride = GetStride(bfView, bv_idx, accessor, ac_idx, sizeof(float));
			int binOffset = bv_offset + ac_offset;


			for (long long j = 0; j < ac_count; ++j)
			{
				float* pos = reinterpret_cast<float*>(binBuffer + binOffset + j * stride);
				ch_list[i].vertices[j].position.x = pos[0];
				ch_list[i].vertices[j].position.y = pos[1];
				ch_list[i].vertices[j].position.z = pos[2];
				ch_list[i].vertices[j].position.w = 1;
			}
		}

		{// - Normal - 
			long long ac_idx = gltfDesc.meshList[i].normal_idx;
			long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();

			long long ac_count = accessor[ac_idx]["count"].get<long long>();

			long long bv_offset = bfView[bv_idx]["byteOffset"].get<long long>();
			long long ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<long long>();
			}

			int stride = GetStride(bfView, bv_idx, accessor, ac_idx, sizeof(float));
			int binOffset = bv_offset + ac_offset;


			for (long long j = 0; j < ac_count; ++j)
			{
				float* pos = reinterpret_cast<float*>(binBuffer + binOffset + j * stride);
				ch_list[i].vertices[j].normal.x = pos[0];
				ch_list[i].vertices[j].normal.y = pos[1];
				ch_list[i].vertices[j].normal.z = pos[2];
				ch_list[i].vertices[j].normal.w = 0;

			}
		}

		{// - texCoord -
			long long ac_idx = gltfDesc.meshList[i].texCoord_idx;
			long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();

			long long ac_count = accessor[ac_idx]["count"].get<long long>();

			long long bv_offset = bfView[bv_idx]["byteOffset"].get<long long>();
			long long ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<long long>();
			}

			int stride = GetStride(bfView, bv_idx, accessor, ac_idx, sizeof(float));
			int binOffset = bv_offset + ac_offset;


			for (long long j = 0; j < ac_count; ++j)
			{
				float* pos = reinterpret_cast<float*>(binBuffer + binOffset + j * stride);
				ch_list[i].vertices[j].texCoord.x = pos[0];
				ch_list[i].vertices[j].texCoord.y = pos[1];
			}
		}

		{// - Index -
			long long ac_idx = gltfDesc.meshList[i].indices_idx;
			long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();

			long long ac_count = accessor[ac_idx]["count"].get<long long>();

			long long bv_offset = bfView[bv_idx]["byteOffset"].get<long long>();
			long long ac_offset = 0;
			if (accessor[ac_idx].contains("byteOffset"))
			{
				ac_offset = accessor[ac_idx]["byteOffset"].get<long long>();
			}

			int stride = GetStride(bfView, bv_idx, accessor, ac_idx, sizeof(uint32_t));
			int binOffset = bv_offset + ac_offset;


			for (long long j = 0; j < ac_count; ++j)
			{
				uint32_t* pos = reinterpret_cast<uint32_t*>(binBuffer + binOffset + j * stride);
				ch_list[i].indices[j] = pos[0];
			}
		}


		{ // - textures -
			long long mat_idx = gltfDesc.meshList[i].material_idx;
			long long tex_idx = ma_list[mat_idx].baseTexture;
			long long sam_idx = tex_list[tex_idx].sampler;
			
			ch_list[i].filePath = tex_list[tex_idx].filePath;
			ch_list[i].metallic = ma_list[mat_idx].metallic;
			ch_list[i].roughness = ma_list[mat_idx].roughness;
			ch_list[i].sampler = sam_list[sam_idx];
		}
	}

	// - Extract node -

	const auto& node = j["nodes"];
	long long node_size = node.size();
	nd_listSize = node_size;

	nd_list = new Nodes_Desc[node_size];

	const int MAT4_SIZE = 16;
	const int VEC4_SIZE = 4;

	for (long long i = 0; i < node_size; ++i)
	{
		if (node[i].contains("children"))
		{
			int ch_size = node[i]["children"].size();
			nd_list[i].children = new int[ch_size];
			nd_list[i].childrenSize = ch_size;

			for (int j = 0; j < ch_size; ++j)
			{
				nd_list[i].children[j] = node[i]["children"][j].get<int>();
			}
		}

		if (node[i].contains("matrix"))
		{
			nd_list[i].hasMat = true;
			for (int j = 0; j < MAT4_SIZE; ++j)
			{
				nd_list[i].mat[j] = node[i]["matrix"][j].get<float>();
			}
		}

		if (node[i].contains("mesh"))
		{
			nd_list[i].mesh = node[i]["mesh"].get<int>();
		}

		if (node[i].contains("skin"))
		{
			nd_list[i].skin = node[i]["skin"].get<int>();
		}

		if (node[i].contains("rotation"))
		{
			nd_list[i].hasRotation = true;
			nd_list[i].rotation.x = node[i]["rotation"][0].get<float>();
			nd_list[i].rotation.y = node[i]["rotation"][1].get<float>();
			nd_list[i].rotation.z = node[i]["rotation"][2].get<float>();
			nd_list[i].rotation.w = node[i]["rotation"][3].get<float>();
		}

		if (node[i].contains("scale"))
		{
			nd_list[i].hasScale = true;
			nd_list[i].scale.x = node[i]["scale"][0].get<float>();
			nd_list[i].scale.y = node[i]["scale"][1].get<float>();
			nd_list[i].scale.z = node[i]["scale"][2].get<float>();
			nd_list[i].scale.w = 0;
		}

		if (node[i].contains("translation"))
		{
			nd_list[i].hasTranslation = true;
			nd_list[i].translation.x = node[i]["translation"][0].get<float>();
			nd_list[i].translation.y = node[i]["translation"][1].get<float>();
			nd_list[i].translation.z = node[i]["translation"][2].get<float>();
			nd_list[i].translation.w = 0;
		}
	}

	// - free local variables -

	delete[] gltfDesc.bufferList;
	delete[] gltfDesc.meshList;

	delete[] tex_list;
	delete[] ma_list;
	delete[] sam_list;
	
	delete[] binBuffer;

	gltfDesc.bufferList = nullptr;
	gltfDesc.meshList = nullptr;

	binBuffer = nullptr;

	tex_list = nullptr;
	sam_list = nullptr;
	ma_list = nullptr;

	return true;
}

CharactorMesh_Desc* Parser::GetMeshDesc(void) const
{
	return ch_list;
}

long long Parser::GetMeshCount(void) const
{
	return ch_listSize;
}

Nodes_Desc* Parser::GetNodeDesc(void) const
{
	return nd_list;
}

long long Parser::GetNodeCount(void) const
{
	return nd_listSize;
}

int Parser::GetRootNodeIdx(void) const
{
	return root_idx;
}