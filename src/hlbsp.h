// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include "map.h"
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
	// v31
	LUMP_CLIPNODES2		= 0,
	LUMP_CLIPNODES3		= 1,
	HEADER_LUMPS_31		= 2,
	// extra
	LUMP_LIGHTVECS		= 0,	// deluxemap data
	LUMP_FACEINFO		= 1,	// landscape and lightmap resolution info
	LUMP_CUBEMAPS		= 2,	// cubemap description
	LUMP_VERTNORMALS	= 3,	// phong shaded vertex normals
	LUMP_LEAF_LIGHTING	= 4,	// contain compressed light cubes per empty leafs
	LUMP_WORLDLIGHTS	= 5,	// list of all the virtual and real lights (used to relight models in-game)
	LUMP_COLLISION		= 6,	// physics engine collision hull dump
	LUMP_AINODEGRAPH	= 7,	// node graph that stored into the bsp
	EXTRA_LUMPS			= 12,
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
	IDEXTRAHEADER	= (('H'<<24)+('S'<<16)+('A'<<8)+'X'), // little-endian "XASH"
	EXTRA_VERSION	= 4,

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

struct dheader31_t
{
	dlump_t	lumps[HEADER_LUMPS_31];
};

struct dextrahdr_t
{
	int	id;	// must be little endian XASH
	int	version;
	dlump_t	lumps[EXTRA_LUMPS];
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
	int16_t	flags;
	int16_t	faceInfo;		// xash extension
};

struct dplane_t
{
	vec3_t	normal;
	float	dist;
	int32_t	type;
};

struct dfaceinfo_t
{
	char		landname[16];	// name of description in mapname_land.txt
	uint16_t	textureStep;	// default is 16, pixels\luxels ratio
	uint16_t	maxExtent;		// default is 16, subdivision step ((texture_step * max_extent) - texture_step)
	int16_t		groupid;		// to determine equal landscapes from various groups, -1 - no group
};

struct mip_t
{
	char		name[16];
	uint32_t	width;
	uint32_t	height;
	uint32_t	offsets[4];	// four mip maps stored
};

}//hlbsp
