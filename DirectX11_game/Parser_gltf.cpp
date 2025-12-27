#include"Parser_gltf.h"

#include<json.hpp>
#include<iostream>
#include<fstream>


typedef nlohmann::json json;

inline int GetComponentSize(int compType) {
	if (compType == 5126) return sizeof(float);      // FLOAT
	if (compType == 5125) return sizeof(uint32_t);   // UNSIGNED_INT
	if (compType == 5123) return sizeof(uint16_t);   // UNSIGNED_SHORT
	if (compType == 5121) return sizeof(uint8_t);    // UNSIGNED_BYTE
	return 0;
}

inline int GetStride(const json& bfView, int bv_idx, const json& accessor, int ac_idx) {
	if (bfView[bv_idx].contains("byteStride")) {
		return bfView[bv_idx]["byteStride"].get<int>();
	}
	else {
		std::string type = accessor[ac_idx]["type"].get<std::string>();
		int ac_kind = (type == "VEC4") ? 4 : (type == "VEC3") ? 3 : (type == "VEC2") ? 2 : 1;
		int compType = accessor[ac_idx]["componentType"].get<int>();
		return GetComponentSize(compType) * ac_kind;
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
    std::fstream file(gltf_file);
    if (!file.is_open()) {
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

    // --- Buffers ---
    if (!j.contains("buffers")) {
        std::cerr << "Error : (GLTF-BUFFERS) File doesn't contains buffers\n";
        return false;
    }
    const auto& buffer = j["buffers"];
    size_t bufferSize = buffer.size();
    Buffers* bfList = new Buffers[bufferSize];
    for (int i = 0; i < bufferSize; ++i) {
        bfList[i].binFileName = buffer[i]["uri"].get<std::string>();
        bfList[i].byteLength = buffer[i]["byteLength"].get<int>();
    }
    std::ifstream bFile(bfList->binFileName, std::ios::binary);
    if (!bFile.is_open()) {
        std::cerr << "Error : (GLTF-BINBUFFER) File doesn't contains binBuffer\n";
        return false;
    }
    binBuffer = new char[bfList->byteLength];
    bFile.read(binBuffer, bfList->byteLength);
    bFile.close();
    gltfDesc.bufferList = bfList;

    // --- Meshes ---
    if (!j.contains("meshes")) {
        std::cerr << "Error : (GLTF-MESHES) File doesn't contians meshes\n";
        return false;
    }
    const auto& mesh = j["meshes"];
    size_t meshSize = mesh.size();
    Meshes* mhList = new Meshes[meshSize];
    ch_list = new CharactorMesh_Desc[meshSize];
    ch_listSize = meshSize;

    for (int i = 0; i < meshSize; ++i) {
        auto& primitive = mesh[i]["primitives"][0];
        auto& attributes = primitive["attributes"];

        mhList[i].indices_idx = primitive["indices"].get<long long>();
        mhList[i].material_idx = primitive["material"].get<long long>();
        mhList[i].position_idx = attributes["POSITION"].get<long long>();
        mhList[i].normal_idx = attributes.contains("NORMAL") ? attributes["NORMAL"].get<long long>() : -1;
        mhList[i].texCoord_idx = attributes.contains("TEXCOORD_0") ? attributes["TEXCOORD_0"].get<long long>() : -1;
        mhList[i].joint_idx = attributes.contains("JOINTS_0") ? attributes["JOINTS_0"].get<long long>() : -1;
    }
    gltfDesc.meshList = mhList;

    // --- Accessors ---
    const auto& accessor = j["accessors"];
    for (long long i = 0; i < meshSize; ++i) {
        long long v_ac_idx = gltfDesc.meshList[i].position_idx;
        long long i_ac_idx = gltfDesc.meshList[i].indices_idx;
        long long j_ac_idx = gltfDesc.meshList[i].joint_idx;
        long long u_ac_idx = gltfDesc.meshList[i].texCoord_idx;

        long long ver_size = accessor[v_ac_idx]["count"].get<long long>();
        long long ind_size = accessor[i_ac_idx]["count"].get<long long>();
        long long joi_size = (j_ac_idx != -1) ? accessor[j_ac_idx]["count"].get<long long>() : 0;
        long long uvc_size = (u_ac_idx != -1) ? accessor[u_ac_idx]["count"].get<long long>() : 0;

        ch_list[i].vertexCount = ver_size;
        ch_list[i].indexCount = ind_size;
        ch_list[i].jointCount = joi_size;
        ch_list[i].uvCount = uvc_size;

        ch_list[i].vertices = new Vertex[ver_size];
        ch_list[i].indices = new uint32_t[ind_size];
        if (joi_size > 0) ch_list[i].joints = new UVec4_t[joi_size];
        if (uvc_size > 0) ch_list[i].uvCoord = new Vec2_t[uvc_size];
    }

    // --- Root Node ---
    const auto& scene = j["scenes"];
    root_idx = scene[0]["nodes"][0].get<int>();

    // --- Textures / Materials / Samplers ---
    Textures* tex_list = nullptr;
    Materials* ma_list = nullptr;
    Samplers* sam_list = nullptr;
    long long tex_listSize = 0, ma_listSize = 0, sam_listSize = 0;

    if (j.contains("textures") && j.contains("images")) {
        const auto& texture = j["textures"];
        const auto& image = j["images"];
        tex_listSize = texture.size();
        tex_list = new Textures[tex_listSize];
        for (long long i = 0; i < tex_listSize; ++i) {
            tex_list[i].filePath = image[i]["uri"].get<std::string>();
            tex_list[i].sampler = texture[i].contains("sampler") ? texture[i]["sampler"].get<long long>() : 0;
        }
    }

    if (j.contains("materials")) {
        const auto& material = j["materials"];
        ma_listSize = material.size();
        ma_list = new Materials[ma_listSize];
        for (long long i = 0; i < ma_listSize; ++i) {
            if (material[i].contains("pbrMetallicRoughness")) {
                const auto& pbr = material[i]["pbrMetallicRoughness"];
                ma_list[i].baseTexture = (pbr.contains("baseColorTexture")) ? pbr["baseColorTexture"]["index"].get<long long>() : -1;
                ma_list[i].metallic = (pbr.contains("metallicFactor")) ? pbr["metallicFactor"].get<float>() : 0.0f;
                ma_list[i].roughness = (pbr.contains("roughnessFactor")) ? pbr["roughnessFactor"].get<float>() : 1.0f;
            }
            else {
                ma_list[i].baseTexture = -1;
                ma_list[i].metallic = 0.0f;
                ma_list[i].roughness = 1.0f;
            }
        }
    }

    if (j.contains("samplers")) {
        const auto& sampler = j["samplers"];
        sam_listSize = sampler.size();
        sam_list = new Samplers[sam_listSize];
        for (long long i = 0; i < sam_listSize; ++i) {
            sam_list[i].magFilter = sampler[i].contains("magFilter") ? sampler[i]["magFilter"].get<int>() : 9729;
            sam_list[i].minFilter = sampler[i].contains("minFilter") ? sampler[i]["minFilter"].get<int>() : 9729;
            sam_list[i].wrapS = sampler[i].contains("wrapS") ? sampler[i]["wrapS"].get<int>() : 10497;
            sam_list[i].wrapT = sampler[i].contains("wrapT") ? sampler[i]["wrapT"].get<int>() : 10497;
        }
    }

    // --- Extract Vertex & Index ---
    const auto& bfView = j["bufferViews"];
    for (long long i = 0; i < meshSize; ++i) {
        // Position
        {
            long long ac_idx = gltfDesc.meshList[i].position_idx;
            long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();
            long long ac_count = accessor[ac_idx]["count"].get<long long>();
            long long bv_offset = bfView[bv_idx].contains("byteOffset") ? bfView[bv_idx]["byteOffset"].get<long long>() : 0;
            long long ac_offset = accessor[ac_idx].contains("byteOffset") ? accessor[ac_idx]["byteOffset"].get<long long>() : 0;
            int stride = GetStride(bfView, bv_idx, accessor, ac_idx);
            int binOffset = bv_offset + ac_offset;
            for (long long j = 0; j < ac_count; ++j) {
                float* pos = reinterpret_cast<float*>(binBuffer + binOffset + j * stride);
                ch_list[i].vertices[j].position.x = pos[0];
                ch_list[i].vertices[j].position.y = pos[1];
                ch_list[i].vertices[j].position.z = pos[2];
                ch_list[i].vertices[j].position.w = 1;
            }
        }
        // Normal
        if (gltfDesc.meshList[i].normal_idx != -1) {
            long long ac_idx = gltfDesc.meshList[i].normal_idx;
            long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();
            long long ac_count = accessor[ac_idx]["count"].get<long long>();
            long long bv_offset = bfView[bv_idx].contains("byteOffset") ? bfView[bv_idx]["byteOffset"].get<long long>() : 0;
            long long ac_offset = accessor[ac_idx].contains("byteOffset") ? accessor[ac_idx]["byteOffset"].get<long long>() : 0;
            int stride = GetStride(bfView, bv_idx, accessor, ac_idx);
            int binOffset = bv_offset + ac_offset;
            for (long long j = 0; j < ac_count; ++j) {
                float* pos = reinterpret_cast<float*>(binBuffer + binOffset + j * stride);
                ch_list[i].vertices[j].normal.x = pos[0];
                ch_list[i].vertices[j].normal.y = pos[1];
                ch_list[i].vertices[j].normal.z = pos[2];
                ch_list[i].vertices[j].normal.w = 0;
            }
        }

        // TexCoord
        if (gltfDesc.meshList[i].texCoord_idx != -1) {
            long long ac_idx = gltfDesc.meshList[i].texCoord_idx;
            long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();
            long long ac_count = accessor[ac_idx]["count"].get<long long>();
            long long bv_offset = bfView[bv_idx].contains("byteOffset") ? bfView[bv_idx]["byteOffset"].get<long long>() : 0;
            long long ac_offset = accessor[ac_idx].contains("byteOffset") ? accessor[ac_idx]["byteOffset"].get<long long>() : 0;
            int stride = GetStride(bfView, bv_idx, accessor, ac_idx);
            int binOffset = bv_offset + ac_offset;
            for (long long j = 0; j < ac_count; ++j) {
                float* pos = reinterpret_cast<float*>(binBuffer + binOffset + j * stride);
                ch_list[i].vertices[j].texCoord.x = pos[0];
                ch_list[i].vertices[j].texCoord.y = pos[1];
            }
        }

        // Indices
        {
            long long ac_idx = gltfDesc.meshList[i].indices_idx;
            long long bv_idx = accessor[ac_idx]["bufferView"].get<long long>();
            long long ac_count = accessor[ac_idx]["count"].get<long long>();
            long long bv_offset = bfView[bv_idx].contains("byteOffset") ? bfView[bv_idx]["byteOffset"].get<long long>() : 0;
            long long ac_offset = accessor[ac_idx].contains("byteOffset") ? accessor[ac_idx]["byteOffset"].get<long long>() : 0;
            int stride = GetStride(bfView, bv_idx, accessor, ac_idx);
            int binOffset = bv_offset + ac_offset;
            int compType = accessor[ac_idx]["componentType"].get<int>();

            for (long long j = 0; j < ac_count; ++j) {
                if (compType == 5125) { // UNSIGNED_INT
                    uint32_t* pos = reinterpret_cast<uint32_t*>(binBuffer + binOffset + j * stride);
                    ch_list[i].indices[j] = pos[0];
                }
                else if (compType == 5123) { // UNSIGNED_SHORT
                    uint16_t* pos = reinterpret_cast<uint16_t*>(binBuffer + binOffset + j * stride);
                    ch_list[i].indices[j] = pos[0];
                }
                else if (compType == 5121) { // UNSIGNED_BYTE
                    uint8_t* pos = reinterpret_cast<uint8_t*>(binBuffer + binOffset + j * stride);
                    ch_list[i].indices[j] = pos[0];
                }
            }
        }

        // Textures
        {
            long long mat_idx = gltfDesc.meshList[i].material_idx;
            if (mat_idx >= 0 && mat_idx < ma_listSize) {
                long long tex_idx = ma_list[mat_idx].baseTexture;
                if (tex_idx >= 0 && tex_idx < tex_listSize) {
                    long long sam_idx = tex_list[tex_idx].sampler;
                    ch_list[i].filePath = tex_list[tex_idx].filePath;
                    ch_list[i].metallic = ma_list[mat_idx].metallic;
                    ch_list[i].roughness = ma_list[mat_idx].roughness;
                    if (sam_idx >= 0 && sam_idx < sam_listSize)
                        ch_list[i].sampler = sam_list[sam_idx];
                }
            }
        }
    }

    // --- Nodes ---
    const auto& node = j["nodes"];
    long long node_size = node.size();
    nd_listSize = node_size;
    nd_list = new Nodes_Desc[node_size];
    const int MAT4_SIZE = 16;

    for (long long i = 0; i < node_size; ++i) {
        if (node[i].contains("children")) {
            int ch_size = node[i]["children"].size();
            nd_list[i].children = new int[ch_size];
            nd_list[i].childrenSize = ch_size;
            for (int j = 0; j < ch_size; ++j) {
                nd_list[i].children[j] = node[i]["children"][j].get<int>();
            }
        }
        if (node[i].contains("matrix")) {
            nd_list[i].hasMat = true;
            for (int j = 0; j < MAT4_SIZE; ++j) {
                nd_list[i].mat[j] = node[i]["matrix"][j].get<float>();
            }
        }
        if (node[i].contains("mesh")) {
            nd_list[i].mesh = node[i]["mesh"].get<int>();
        }
        if (node[i].contains("skin")) {
            nd_list[i].skin = node[i]["skin"].get<int>();
        }
        if (node[i].contains("rotation")) {
            nd_list[i].hasRotation = true;
            nd_list[i].rotation.x = node[i]["rotation"][0].get<float>();
            nd_list[i].rotation.y = node[i]["rotation"][1].get<float>();
            nd_list[i].rotation.z = node[i]["rotation"][2].get<float>();
            nd_list[i].rotation.w = node[i]["rotation"][3].get<float>();
        }
        if (node[i].contains("scale")) {
            nd_list[i].hasScale = true;
            nd_list[i].scale.x = node[i]["scale"][0].get<float>();
            nd_list[i].scale.y = node[i]["scale"][1].get<float>();
            nd_list[i].scale.z = node[i]["scale"][2].get<float>();
            nd_list[i].scale.w = 0;
        }
        if (node[i].contains("translation")) {
            nd_list[i].hasTranslation = true;
            nd_list[i].translation.x = node[i]["translation"][0].get<float>();
            nd_list[i].translation.y = node[i]["translation"][1].get<float>();
            nd_list[i].translation.z = node[i]["translation"][2].get<float>();
            nd_list[i].translation.w = 0;
        }
    }

    // --- free local variables ---
    delete[] gltfDesc.bufferList;
    delete[] gltfDesc.meshList;
    delete[] tex_list;
    delete[] ma_list;
    delete[] sam_list;
    delete[] binBuffer;

    gltfDesc.bufferList = nullptr;
    gltfDesc.meshList = nullptr;
    binBuffer = nullptr;

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