// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <vector>
#include "Texture.h"

enum bspIdents
{
	HLBSP_VERSION = 30,	// half-life regular version
	XTBSP_VERSION = 31,	// extended lightmaps and expanded clipnodes limit

	VBSP_IDENT = (('P' << 24) + ('S' << 16) + ('B' << 8) + 'V'), //source engine
};

struct vec3_t
{
	float x, y, z;
};

class WadFile;

class Map
{
public:
	struct LoadConfig
	{
		std::string gamePath;
		bool skipSky = false;
		int lightmapSize = 1024;
		int lstyle = -1;
		bool lstylesAll = false;
		bool uint16Inds = false;
		bool allTextures = false;
	};
	bool load(const char *path, const char *name, LoadConfig *config = nullptr);

	struct vert_t
	{
		vec3_t pos;
		float uv[2];
		float uv2[2];
	};
	struct submesh_t
	{
		int offset;
		int count;
		int material;
	};
	struct model_t
	{
		int offset;
		int count;
		int vertOffset;
		int vertCount;
		std::vector<submesh_t> submeshes;
	};
	struct material_t
	{
		std::string name;
		int texture = -1;
		bool alphaMask = false;
	};

	std::vector<vert_t> vertices;
	std::vector<uint16_t> indices16;
	std::vector<uint32_t> indices32;
	// model can contain multiple meshes
	std::vector<std::vector<model_t> > models;
	std::vector<Texture> textures;
	std::vector<material_t> materials;

private:
	bool load_hlbsp(FILE *f, const char *name, LoadConfig *config = nullptr);
	bool load_vbsp(FILE *f, const char *name, LoadConfig *config = nullptr);

	void hlbsp_loadTextures(FILE *f, int fileofs, int filelen, std::vector<WadFile> wads);
	void parseEntities(const char *src, size_t size);

	std::vector<std::string> wadNames;
};
