// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "gltf_export.h"
#include <fstream>
#include "nlohmann/json.hpp"
#include "map.h"
#include "bsp-converter.h"
#include <cfloat>

#ifdef _WIN32
#include <direct.h>
#endif

namespace gltf
{

bool exportMap(const std::string &name, Map &map, bool verbose)
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
	const int vertBufferOffset = 0;
	const size_t dispVertBufferOffset = map.vertices.size() * sizeof(map.vertices[0]);
	const size_t indsBufferOffset = dispVertBufferOffset + map.dispVertices.size() * sizeof(map.dispVertices[0]);
	const size_t indSize = map.indices16.size() ? sizeof(uint16_t) : sizeof(uint32_t);
	const int indType = map.indices16.size() ? UNSIGNED_SHORT : UNSIGNED_INT;

	for (int i = 0; i < map.models.size(); i++)
	{
		int modelNodeId = nodeId;
		nodeId++;
		nodes[modelNodeId] = { {"name",std::string("*") + std::to_string(i)} };
		nodes[0]["children"].push_back(modelNodeId);

		bool singleMesh = (map.models[i].meshes.size() + map.models[i].dispMeshes.size() == 1);

		if (singleMesh)
		{
			nodes[modelNodeId]["mesh"] = meshId;
			const vec3_t &p = map.models[i].position;
			if (p.x != 0 && p.y != 0 && p.z != 0)
				nodes[modelNodeId]["translation"] = { p.x, p.y, p.z };
		}

		auto writeMesh = [&](json &mesh, const Map::mesh_t &part, size_t vertsOffset, size_t vertSize)
		{
			bufferViews[bufferViewId + 0] = { {"buffer", 0}, {"byteOffset", indsBufferOffset + part.offset * indSize}, {"byteLength", part.count * indSize}, {"target", ELEMENT_ARRAY_BUFFER} };
			bufferViews[bufferViewId + 1] = { {"buffer", 0}, {"byteOffset", vertsOffset + part.vertOffset * vertSize}, {"byteLength", part.vertCount * vertSize},{"byteStride", vertSize}, {"target", ARRAY_BUFFER} };
			vec3_t bmin{ FLT_MAX, FLT_MAX, FLT_MAX };
			vec3_t bmax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
			for (int j = 0; j < part.vertCount; j++)
			{
				vec3_t v = (vertsOffset == 0) ? map.vertices[part.vertOffset + j].pos : map.dispVertices[part.vertOffset + j].pos;
				bmin.x = fmin(bmin.x, v.x);
				bmin.y = fmin(bmin.y, v.y);
				bmin.z = fmin(bmin.z, v.z);
				bmax.x = fmax(bmax.x, v.x);
				bmax.y = fmax(bmax.y, v.y);
				bmax.z = fmax(bmax.z, v.z);
			}
			modelAccessorId = accessorId;
			accessors[accessorId + 0] = { {"bufferView",bufferViewId + 1},{"byteOffset",0},{"componentType",FLOAT},{"count",part.vertCount},{"type","VEC3"}, {"min",{bmin.x, bmin.y, bmin.z}}, {"max",{bmax.x, bmax.y, bmax.z}} };
			accessors[accessorId + 1] = { {"bufferView",bufferViewId + 1},{"byteOffset",12},{"componentType",FLOAT},{"count",part.vertCount},{"type","VEC3"} };
			accessors[accessorId + 2] = { {"bufferView",bufferViewId + 1},{"byteOffset",24},{"componentType",FLOAT},{"count",part.vertCount},{"type","VEC2"} };
			accessors[accessorId + 3] = { {"bufferView",bufferViewId + 1},{"byteOffset",32},{"componentType",FLOAT},{"count",part.vertCount},{"type","VEC2"} };
			accessorId += 4;
			if (vertsOffset != 0)
			{
				accessors[accessorId] = { {"bufferView",bufferViewId + 1},{"byteOffset",40},{"componentType",FLOAT},{"count",part.vertCount},{"type","VEC4"} };
				accessorId++;
			}

			for (int j = 0; j < part.submeshes.size(); j++)
			{
				mesh["primitives"][j] = {
					{"attributes", {{"POSITION",modelAccessorId + 0}, {"NORMAL",modelAccessorId + 1}, {"TEXCOORD_0",modelAccessorId + 2}, {"TEXCOORD_1",modelAccessorId + 3}}},
					{"indices", accessorId},
					{"material", part.submeshes[j].material}
				};
				if(vertsOffset != 0)
				{
					mesh["primitives"][j]["attributes"]["COLOR_0"] = modelAccessorId + 4;
				}
				accessors[accessorId] = { {"bufferView", bufferViewId}, { "byteOffset", (part.submeshes[j].offset - part.offset) * indSize }, { "componentType", indType }, { "count", part.submeshes[j].count }, { "type","SCALAR" } };
				accessorId++;
			}
			bufferViewId += 2;
		};

		for (int mmi = 0; mmi < map.models[i].meshes.size(); mmi++)
		{
			const Map::mesh_t &part = map.models[i].meshes[mmi];
			if (part.vertCount == 0)
				continue;

			meshes[meshId] = { {"primitives",json::array()} };
			if (!singleMesh)
			{
				std::string meshName = name + "_model" + std::to_string(i) + "_mesh" + std::to_string(mmi);
				nodes[nodeId] = { {"name", part.name.empty() ? meshName : part.name}, {"mesh", meshId} };
				nodes[modelNodeId]["children"].push_back(nodeId);
				nodeId++;
				meshes[meshId]["name"] = meshName;
			}
			else
			{
				meshes[meshId]["name"] = name + "_model" + std::to_string(i) + "_mesh";
			}

			writeMesh(meshes[meshId], part, vertBufferOffset, sizeof(map.vertices[0]));
			meshId++;
		}

		for (int dmi = 0; dmi < map.models[i].dispMeshes.size(); dmi++)
		{
			const Map::mesh_t &part = map.models[i].dispMeshes[dmi];
			if (part.vertCount == 0)
				continue;

			meshes[meshId] = { {"primitives",json::array()} };
			if (!singleMesh)
			{
				std::string meshName = name + "_model" + std::to_string(i) + "_dispMesh" + std::to_string(dmi);
				nodes[nodeId] = { {"name", part.name.empty() ? meshName : part.name}, {"mesh", meshId} };
				nodes[modelNodeId]["children"].push_back(nodeId);
				nodeId++;
				meshes[meshId]["name"] = meshName;
			}
			else
			{
				meshes[meshId]["name"] = name + "_model" + std::to_string(i) + "_dispMesh";
			}

			writeMesh(meshes[meshId], part, dispVertBufferOffset, sizeof(map.dispVertices[0]));
			meshId++;
		}
	}

	auto &textures = j["textures"];
	auto &images = j["images"];
	//TODO: write only used textures
	int lmapTexIndex = (int)map.textures.size();
	for (size_t i = 0; i < map.textures.size(); i++)
	{
		std::string texturePath = std::string("textures/") + map.textures[i].name + ".png";
		if (map.textures[i].data.size())
			map.textures[i].save(texturePath.c_str(), verbose);

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

		materials[i]["pbrMetallicRoughness"] = {
			{"metallicFactor", 0}
		};
		if (mat.texture != -1)
		{
			materials[i]["pbrMetallicRoughness"]["baseColorTexture"] = { {"index", mat.texture} };
		}
		if (mat.lightmapped)
			materials[i]["extensions"] = { {"EXT_materials_lightmap",{{"lightmapTexture", { {"index", lmapTexIndex}, {"texCoord", 1} }}}} };
	}

	images[lmapTexIndex] = { {"uri", name + "_lightmap0.png"} };
	textures[lmapTexIndex] = { {"source", lmapTexIndex} };

	size_t vertsLen = map.vertices.size() * sizeof(map.vertices[0]) + map.dispVertices.size() * sizeof(map.dispVertices[0]);
	size_t indsLen = 0;

	std::string bufferName = name + ".bin";

	if (verbose)
		printf("Writing: %s\n", bufferName.c_str());
	std::ofstream bufferFile(bufferName, std::ios_base::binary);
	if(map.vertices.size())
		bufferFile.write((char *)&map.vertices[0], map.vertices.size() * sizeof(map.vertices[0]));

	if(map.dispVertices.size())
		bufferFile.write((char *)&map.dispVertices[0], map.dispVertices.size() * sizeof(map.dispVertices[0]));

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

	if (verbose)
		printf("Writing: %s.gltf\n", name.c_str());
	std::ofstream o(name + ".gltf");
	// enable 'pretty printing'
	o << std::setw(4);
	o << j << std::endl;
	o.close();

	return true;
}

}//gltf
