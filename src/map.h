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

struct vec2i_t
{
	int x, y;
};

struct vec2_t
{
	union
	{
		struct
		{
			float x, y;
		};
		float v[2];
	};

	static vec2_t lerp(const vec2_t &v1, const vec2_t &v2, float k)
	{
		float ik = 1.0f - k;
		return{ v1.x * ik + v2.x * k, v1.y * ik + v2.y * k };
	}
};

struct vec3_t
{
	float x, y, z;

	float dot(const vec3_t &v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	double dot_d(const vec3_t &v) const
	{
		return x * (double)v.x + y * (double)v.y + z * (double)v.z;
	}

	vec3_t cross(const vec3_t &v) const
	{
		return {
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x
		};
	}

	vec3_t &normalize()
	{
		float l = sqrtf(dot(*this));
		x /= l;
		y /= l;
		z /= l;
		return *this;
	}

	float dist2(const vec3_t &v) const
	{
		float dx = v.x - x;
		float dy = v.y - y;
		float dz = v.z - z;
		return (dx * dx + dy * dy + dz * dz);
	}

	vec3_t &operator *= (float s)
	{
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}

	static vec3_t lerp(const vec3_t &v1, const vec3_t &v2, float k)
	{
		float ik = 1.0f - k;
		return{ v1.x * ik + v2.x * k, v1.y * ik + v2.y * k , v1.z * ik + v2.z * k };
	}
};

inline vec3_t operator+ (const vec3_t &v1, const vec3_t &v2)
{
	return { v1.x + v2.x,v1.y + v2.y, v1.z + v2.z };
}
inline vec3_t operator- (const vec3_t &v1, const vec3_t &v2)
{
	return { v1.x - v2.x,v1.y - v2.y, v1.z - v2.z };
}

class WadFile;

class Map
{
public:
	struct LoadConfig
	{
		std::string gamePath;
		bool skipSky = false;
		int lightmapSize = 2048;
		int lstyle = -1;
		bool lstylesAll = false;
		bool uint16Inds = false;
		bool allTextures = false;
	};
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

	void hlbsp_loadTextures(FILE *f, int fileofs, int filelen, std::vector<WadFile> wads);
	void parseEntities(const char *src, size_t size);

	std::vector<std::string> wadNames;
};
