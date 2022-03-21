// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include "Texture.h"
#include <stdint.h>
#include <vector>

#define BIT(n) (1U << (n))

namespace hlbsp
{

enum Lumps
{
	LUMP_ENTITIES		= 0,
	LUMP_PLANES			= 1,
	LUMP_TEXTURES		= 2,
	LUMP_VERTEXES		= 3,
	LUMP_VISIBILITY		= 4,
	LUMP_NODES			= 5,
	LUMP_TEXINFO		= 6,
	LUMP_FACES			= 7,
	LUMP_LIGHTING		= 8,
	LUMP_CLIPNODES		= 9,
	LUMP_LEAFS			= 10,
	LUMP_MARKSURFACES	= 11,
	LUMP_EDGES			= 12,
	LUMP_SURFEDGES		= 13,
	LUMP_MODELS			= 14,
	HEADER_LUMPS		= 15,
};

enum class SurfaceFlags
{
	NOCULL		= BIT(0),	// two-sided polygon (e.g. 'water4b')
	PLANEBACK	= BIT(1),	// plane should be negated
	DRAWSKY		= BIT(2),	// sky surface
	WATERCSG	= BIT(3),	// culled by csg (was SURF_DRAWSPRITE)
	DRAWTURB	= BIT(4),	// warp surface
	DRAWTILED	= BIT(5),	// face without lighmap
	CONVEYOR	= BIT(6),	// scrolled texture (was SURF_DRAWBACKGROUND)
	UNDERWATER	= BIT(7),	// caustics
	TRANSPARENT	= BIT(8)	// it's a transparent texture (was SURF_DONTWARP)
};

enum
{
	HLBSP_VERSION	= 30,	// half-life regular version
	LM_SAMPLE_SIZE	= 16,

	MAX_MAP_HULLS	= 4,
	LM_STYLES		= 4
};

struct dlump_t
{
	int32_t	fileofs;
	int32_t	filelen;
};

struct dheader_t
{
	int32_t	version;
	dlump_t	lumps[HEADER_LUMPS];
};

struct vec3_t
{
	float x, y, z;
};

struct dmodel_t
{
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	origin;
	int32_t	headnode[MAX_MAP_HULLS];
	int32_t	visleafs;
	int32_t	firstface;
	int32_t	numfaces;
};

struct dface_t
{
	uint16_t planenum;
	int16_t side;

	int32_t	firstedge;
	int16_t	numedges;
	int16_t	texinfo;

	// lighting info
	uint8_t	styles[LM_STYLES];
	int32_t	lightofs;		// start of [numstyles*surfsize] samples
};

// NOTE: that edge 0 is never used, because negative edge nums
// are used for counterclockwise use of the edge in a face
struct dedge_t
{
	uint16_t v[2];			// vertex numbers
};

struct dtexinfo_t
{
	float	vecs[2][4];		// texmatrix [s/t][xyz offset]
	int32_t	miptex;
	int32_t	flags;
};

struct mip_t
{
	char		name[16];
	uint32_t	width;
	uint32_t	height;
	uint32_t	offsets[4];	// four mip maps stored
};

struct LoadConfig
{
	bool skipSky = false;
	int lightmapSize = 1024;
};

class Map
{
public:
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

	std::vector<vert_t> vertices;
	std::vector<uint32_t> indices;
	std::vector<model_t> models;

	std::vector<dface_t> faces;
	std::vector<int> surfedges;
	std::vector<dedge_t> edges;
	std::vector<dtexinfo_t> texinfos;
	std::vector<Texture> textures;
	std::vector<uint8_t> lightmapPixels;

private:
	void loadTextures(FILE *f, dlump_t lump);
};

}//hlbsp
