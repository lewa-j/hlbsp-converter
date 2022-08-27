// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <vector>
#include "texture.h"
#include "vector_math.h"
#include "config.h"

enum bspIdents
{
	HLBSP_VERSION = 30,	// half-life regular version
	XTBSP_VERSION = 31,	// extended lightmaps and expanded clipnodes limit

	VBSP_IDENT = (('P' << 24) + ('S' << 16) + ('B' << 8) + 'V'), //source engine
};

class WadFile;

class Map
{
public:
	bool load(const char *path, const char *name, LoadConfig *config = nullptr);

	struct vert_t
	{
		vec3_t pos;
		vec3_t norm;
		vec2_t uv;
		vec2_t uv2;
	};
	struct dispVert_t
	{
		vec3_t pos;
		vec3_t norm;
		vec2_t uv;
		vec2_t uv2;
		vec3_t color;
		float alpha;
	};
	struct submesh_t
	{
		int offset;
		int count;
		int material;
	};
	struct mesh_t
	{
		int offset;
		int count;
		int vertOffset;
		int vertCount;
		std::vector<submesh_t> submeshes;
	};
	struct model_t
	{
		std::vector<mesh_t> meshes;
		std::vector<mesh_t> dispMeshes;
		vec3_t position = { 0,0,0 };
	};
	struct material_t
	{
		std::string name;
		int texture = -1;
		bool alphaMask = false;
		bool lightmapped = false;
	};

	std::vector<vert_t> vertices;
	std::vector<dispVert_t> dispVertices;
	std::vector<uint16_t> indices16;
	std::vector<uint32_t> indices32;
	// model can contain multiple meshes
	std::vector<model_t> models;
	std::vector<Texture> textures;
	std::vector<material_t> materials;

private:
	bool load_hlbsp(FILE *f, const char *name, LoadConfig *config = nullptr);
	bool load_vbsp(FILE *f, const char *name, LoadConfig *config = nullptr);

	void hlbsp_loadTextures(FILE *f, int fileofs, int filelen, std::vector<WadFile> wads, bool verbose);
	void parseEntities(const char *src, size_t size);

	std::vector<std::string> wadNames;
};
