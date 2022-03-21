// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "gltf_export.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <direct.h>

namespace gltf
{

bool ExportMap(const std::string &name, hlbsp::Map &map)
{
	using nlohmann::json;
	json j;
	j["asset"] = { {"version", "2.0"} };
	j["scene"] = 0;
	j["scenes"] = { { { "nodes", {0} } } };
	auto &nodes = j["nodes"];
	nodes[0] = {
		{"name",name},
		{"rotation",{-sqrt(0.5f),0,0,sqrt(0.5f)}},
		{"scale",{0.03,0.03,0.03}}
	};
	auto &meshes = j["meshes"];
	auto &accessors = j["accessors"];
	auto &bufferViews = j["bufferViews"];
	int accessorId = 0;
	int modelAccessorId = 0;
	int bufferViewId = 0;
	const int indsBufferOffset = map.vertices.size() * sizeof(map.vertices[0]);
	for (int i = 0; i < map.models.size(); i++)
	{
		nodes[0]["children"].push_back(i + 1);
		nodes[i + 1] = { {"mesh", i}, {"name",std::string("*") + std::to_string(i)} };

		bufferViews[bufferViewId + 0] = { {"buffer", 0}, {"byteOffset", indsBufferOffset + map.models[i].offset * sizeof(map.indices[0])}, {"byteLength", map.models[i].count * sizeof(map.indices[0])}, {"target", ELEMENT_ARRAY_BUFFER} };
		bufferViews[bufferViewId + 1] = { {"buffer", 0}, {"byteOffset", map.models[i].vertOffset * sizeof(map.vertices[0])}, {"byteLength", map.models[i].vertCount * sizeof(map.vertices[0])},{"byteStride", 28}, {"target", ARRAY_BUFFER} };
		hlbsp::vec3_t bmin{ FLT_MAX, FLT_MAX, FLT_MAX };
		hlbsp::vec3_t bmax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (int j = 0; j < map.models[i].vertCount; j++)
		{
			hlbsp::vec3_t v = map.vertices[map.models[i].vertOffset + j].pos;
			bmin.x = fmin(bmin.x, v.x);
			bmin.y = fmin(bmin.y, v.y);
			bmin.z = fmin(bmin.z, v.z);
			bmax.x = fmax(bmax.x, v.x);
			bmax.y = fmax(bmax.y, v.y);
			bmax.z = fmax(bmax.z, v.z);
		}
		accessors[accessorId + 0] = { {"bufferView",bufferViewId + 1},{"byteOffset",0},{"componentType",FLOAT},{"count",map.models[i].vertCount},{"type","VEC3"}, {"min",{bmin.x, bmin.y, bmin.z}}, {"max",{bmax.x, bmax.y, bmax.z}} };
		accessors[accessorId + 1] = { {"bufferView",bufferViewId + 1},{"byteOffset",12},{"componentType",FLOAT},{"count",map.models[i].vertCount},{"type","VEC2"} };
		accessors[accessorId + 2] = { {"bufferView",bufferViewId + 1},{"byteOffset",20},{"componentType",FLOAT},{"count",map.models[i].vertCount},{"type","VEC2"} };
		modelAccessorId = accessorId;
		accessorId += 3;

		meshes[i] = { {"name", name + "_mesh" + std::to_string(i)}, {"primitives",json::array()} };
		for (int j = 0; j < map.models[i].submeshes.size(); j++)
		{
			meshes[i]["primitives"][j] = {
				{"attributes", {{"POSITION",modelAccessorId + 0}, {"TEXCOORD_0",modelAccessorId + 1}, {"TEXCOORD_1",modelAccessorId + 2}}},
				{"indices", accessorId},
				{"material", map.models[i].submeshes[j].material}
			};
			accessors[accessorId] = { {"bufferView", bufferViewId}, { "byteOffset", (map.models[i].submeshes[j].offset - map.models[i].offset) * sizeof(map.indices[0]) }, { "componentType", UNSIGNED_INT }, { "count", map.models[i].submeshes[j].count }, { "type","SCALAR" } };
			accessorId++;
		}
		bufferViewId += 2;
	}

	_mkdir("textures");

	auto &materials = j["materials"];
	auto &textures = j["textures"];
	auto &images = j["images"];
	//TODO: write only used textures
	int lmapTexIndex = map.textures.size();
	for (int i = 0; i < map.textures.size(); i++)
	{
		std::string texturePath = std::string("textures/") + map.textures[i].name + ".png";
		if (map.textures[i].data.size())
			map.textures[i].save(texturePath.c_str());

		images[i] = { {"uri", texturePath} };
		textures[i] = { {"source", i} };
		materials[i] = {
			{"name", map.textures[i].name},
			{"pbrMetallicRoughness",{
				{"baseColorTexture",{{"index",i}}},
				{"metallicFactor", 0}
			}}
			//,{"emissiveTexture", {{"index", lmapTexIndex}, {"texCoord", 1}}},
			//{"emissiveFactor", {1,1,1}}
		};
	}

	images[lmapTexIndex] = { {"uri", name + "_lightmap0.png"} };
	textures[lmapTexIndex] = { {"source", lmapTexIndex} };

	std::string bufferName = name + ".bin";

	int vertsLen = map.vertices.size() * sizeof(map.vertices[0]);
	int indsLen = map.indices.size() * sizeof(map.indices[0]);
	j["buffers"] = { { {"uri", bufferName}, {"byteLength", vertsLen + indsLen} } };

	printf("Writing: %s\n", bufferName.c_str());
	std::ofstream verts(bufferName, std::ios_base::binary);
	verts.write((char *)&map.vertices[0], vertsLen);
	verts.write((char *)&map.indices[0], indsLen);
	verts.close();

	printf("Writing: %s.gltf\n", name.c_str());
	std::ofstream o(name + ".gltf");
	// enable 'pretty printing'
	o << std::setw(4);
	o << j << std::endl;
	o.close();

	return true;
}

}//gltf
