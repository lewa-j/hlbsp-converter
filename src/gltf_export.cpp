// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "gltf_export.h"
#include "bsp-converter.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <direct.h>

namespace gltf
{

bool ExportMap(const std::string &name, Map &map)
{
	using nlohmann::json;
	json j;
	j["asset"] = { {"version", "2.0"}, {"generator", HLBSP_CONVERTER_NAME}};
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
	int meshId = 0;
	int nodeId = 1;
	const int indsBufferOffset = map.vertices.size() * sizeof(map.vertices[0]);
	const int indSize = map.indices16.size() ? sizeof(uint16_t) : sizeof(uint32_t);
	const int indType = map.indices16.size() ? UNSIGNED_SHORT : UNSIGNED_INT;
	for (int i = 0; i < map.models.size(); i++)
	{
		int modelNodeId = nodeId;
		nodeId++;
		nodes[modelNodeId] = { {"name",std::string("*") + std::to_string(i)} };
		nodes[0]["children"].push_back(modelNodeId);

		if (map.models[i].size() == 1)
		{
			nodes[modelNodeId]["mesh"] = meshId;
		}

		for (int mmi = 0; mmi < map.models[i].size(); mmi++)
		{
			meshes[meshId] = { {"primitives",json::array()} };
			if (map.models[i].size() != 1)
			{
				std::string meshName = name + "_mesh" + std::to_string(i) + "_" + std::to_string(mmi);
				nodes[nodeId] = { {"name", meshName}, {"mesh", meshId} };
				nodes[modelNodeId]["children"].push_back(nodeId);
				nodeId++;
				meshes[meshId]["name"] = meshName;
			}
			else
			{
				meshes[meshId]["name"] = name + "_mesh" + std::to_string(i);
			}
			const Map::model_t &model = map.models[i][mmi];

			bufferViews[bufferViewId + 0] = { {"buffer", 0}, {"byteOffset", indsBufferOffset + model.offset * indSize}, {"byteLength", model.count * indSize}, {"target", ELEMENT_ARRAY_BUFFER} };
			bufferViews[bufferViewId + 1] = { {"buffer", 0}, {"byteOffset", model.vertOffset * sizeof(map.vertices[0])}, {"byteLength", model.vertCount * sizeof(map.vertices[0])},{"byteStride", 28}, {"target", ARRAY_BUFFER} };
			vec3_t bmin{ FLT_MAX, FLT_MAX, FLT_MAX };
			vec3_t bmax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
			for (int j = 0; j < model.vertCount; j++)
			{
				vec3_t v = map.vertices[model.vertOffset + j].pos;
				bmin.x = fmin(bmin.x, v.x);
				bmin.y = fmin(bmin.y, v.y);
				bmin.z = fmin(bmin.z, v.z);
				bmax.x = fmax(bmax.x, v.x);
				bmax.y = fmax(bmax.y, v.y);
				bmax.z = fmax(bmax.z, v.z);
			}
			accessors[accessorId + 0] = { {"bufferView",bufferViewId + 1},{"byteOffset",0},{"componentType",FLOAT},{"count",model.vertCount},{"type","VEC3"}, {"min",{bmin.x, bmin.y, bmin.z}}, {"max",{bmax.x, bmax.y, bmax.z}} };
			accessors[accessorId + 1] = { {"bufferView",bufferViewId + 1},{"byteOffset",12},{"componentType",FLOAT},{"count",model.vertCount},{"type","VEC2"} };
			accessors[accessorId + 2] = { {"bufferView",bufferViewId + 1},{"byteOffset",20},{"componentType",FLOAT},{"count",model.vertCount},{"type","VEC2"} };
			modelAccessorId = accessorId;
			accessorId += 3;

			for (int j = 0; j < model.submeshes.size(); j++)
			{
				meshes[meshId]["primitives"][j] = {
					{"attributes", {{"POSITION",modelAccessorId + 0}, {"TEXCOORD_0",modelAccessorId + 1}, {"TEXCOORD_1",modelAccessorId + 2}}},
					{"indices", accessorId},
					{"material", model.submeshes[j].material}
				};
				accessors[accessorId] = { {"bufferView", bufferViewId}, { "byteOffset", (model.submeshes[j].offset - model.offset) * indSize }, { "componentType", indType }, { "count", model.submeshes[j].count }, { "type","SCALAR" } };
				accessorId++;
			}
			meshId++;
			bufferViewId += 2;
		}
	}

	_mkdir("textures");

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
	}

	auto &materials = j["materials"];
	for (int i = 0; i < map.materials.size(); i++)
	{
		const auto &mat = map.materials[i];
		materials[i] = {{"name", mat.name}};

		if (mat.alphaMask)
			materials[i]["alphaMode"] = "MASK";

		if (mat.texture != -1)
		{
			materials[i]["pbrMetallicRoughness"] = {
				{"baseColorTexture",{{"index", mat.texture}}},
				{"metallicFactor", 0}
			};
			if (mat.texture == lmapTexIndex)
				materials[i]["pbrMetallicRoughness"]["baseColorTexture"]["texCoord"] = 1;
		}
		materials[i]["extensions"] = { {"EXT_materials_lightmap",{{"lightmapTexture", { {"index", lmapTexIndex}, {"texCoord", 1} }}}} };
	}

	images[lmapTexIndex] = { {"uri", name + "_lightmap0.png"} };
	textures[lmapTexIndex] = { {"source", lmapTexIndex} };

	int vertsLen = map.vertices.size() * sizeof(map.vertices[0]);
	int indsLen = 0;

	std::string bufferName = name + ".bin";

	printf("Writing: %s\n", bufferName.c_str());
	std::ofstream bufferFile(bufferName, std::ios_base::binary);
	bufferFile.write((char *)&map.vertices[0], vertsLen);
	if (map.indices16.size())
	{
		indsLen = map.indices16.size() * sizeof(map.indices16[0]);
		bufferFile.write((char *)&map.indices16[0], indsLen);
	}
	else
	{
		indsLen = map.indices32.size() * sizeof(map.indices32[0]);
		bufferFile.write((char *)&map.indices32[0], indsLen);
	}
	bufferFile.close();

	j["buffers"] = { { {"uri", bufferName}, {"byteLength", vertsLen + indsLen} } };

	printf("Writing: %s.gltf\n", name.c_str());
	std::ofstream o(name + ".gltf");
	// enable 'pretty printing'
	o << std::setw(4);
	o << j << std::endl;
	o.close();

	return true;
}

}//gltf
