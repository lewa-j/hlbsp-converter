// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include "map.h"
#include <stdint.h>

namespace srcbsp
{

enum
{
	MAXLIGHTMAPS = 4,
};

enum vbspLump
{
	LUMP_ENTITIES = 0,
	LUMP_PLANES = 1,
	LUMP_TEXDATA = 2,
	LUMP_VERTEXES = 3,
	LUMP_VISIBILITY = 4,
	LUMP_NODES = 5,
	LUMP_TEXINFO = 6,
	LUMP_FACES = 7,
	LUMP_LIGHTING = 8,
	LUMP_OCCLUSION = 9,
	LUMP_LEAFS = 10,
	LUMP_FACEIDS = 11,
	LUMP_EDGES = 12,
	LUMP_SURFEDGES = 13,
	LUMP_MODELS = 14,
	LUMP_WORLDLIGHTS = 15,
	LUMP_LEAFFACES = 16,
	LUMP_LEAFBRUSHES = 17,
	LUMP_BRUSHES = 18,
	LUMP_BRUSHSIDES = 19,
	LUMP_AREAS = 20,
	LUMP_AREAPORTALS = 21,
	LUMP_DISPINFO = 26,
	LUMP_ORIGINALFACES = 27,
	LUMP_PHYSDISP = 28,
	LUMP_PHYSCOLLIDE = 29,
	LUMP_VERTNORMALS = 30,
	LUMP_VERTNORMALINDICES = 31,
	LUMP_DISP_LIGHTMAP_ALPHAS = 32,
	LUMP_DISP_VERTS = 33,
	LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,
	LUMP_GAME_LUMP = 35,
	LUMP_LEAFWATERDATA = 36,
	LUMP_PRIMITIVES = 37,
	LUMP_PRIMVERTS = 38,
	LUMP_PRIMINDICES = 39,
	LUMP_PAKFILE = 40,
	LUMP_CLIPPORTALVERTS = 41,
	LUMP_CUBEMAPS = 42,
	LUMP_TEXDATA_STRING_DATA = 43,
	LUMP_TEXDATA_STRING_TABLE = 44,
	LUMP_OVERLAYS = 45,
	LUMP_LEAFMINDISTTOWATER = 46,
	LUMP_FACE_MACRO_TEXTURE_INFO = 47,
	LUMP_DISP_TRIS = 48,
	LUMP_PHYSCOLLIDESURFACE = 49,
	LUMP_WATEROVERLAYS = 50,
	LUMP_LEAF_AMBIENT_INDEX_HDR = 51,
	LUMP_LEAF_AMBIENT_INDEX = 52,
	LUMP_LIGHTING_HDR = 53,
	LUMP_WORLDLIGHTS_HDR = 54,
	LUMP_LEAF_AMBIENT_LIGHTING_HDR = 55,
	LUMP_LEAF_AMBIENT_LIGHTING = 56,
	LUMP_FACES_HDR = 58,
	LUMP_MAP_FLAGS = 59,
	LUMP_OVERLAY_FADES = 60,

	HEADER_LUMPS = 64
};

enum SurfaceFlags
{
	SURF_LIGHT		= 0x0001,	// value will hold the light strength
	SURF_SKY2D		= 0x0002,	// don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
	SURF_SKY		= 0x0004,	// don't draw, but add to skybox
	SURF_WARP		= 0x0008,	// turbulent water warp
	SURF_TRANS		= 0x0010,
	SURF_NOPORTAL	= 0x0020,	// the surface can not have a portal placed on it
	SURF_TRIGGER	= 0x0040,	// FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
	SURF_NODRAW		= 0x0080,	// don't bother referencing the texture
	SURF_HINT		= 0x0100,	// make a primary bsp splitter
	SURF_SKIP		= 0x0200,	// completely ignore, allowing non-closed brushes
	SURF_NOLIGHT	= 0x0400,	// Don't calculate light
	SURF_BUMPLIGHT	= 0x0800,	// calculate three lightmaps for the surface for bumpmapping
	SURF_NOSHADOWS	= 0x1000,	// Don't receive shadows
	SURF_NODECALS	= 0x2000,	// Don't receive decals
	SURF_NOCHOP		= 0x4000,	// Don't subdivide patches on this surface
	SURF_HITBOX		= 0x8000,	// surface is part of a hitbox
};

struct bspLump_t
{
	uint32_t fileofs;
	uint32_t filelen;
	uint32_t version;
	uint32_t uncompressedSize;
};

struct bspHeader_t
{
	uint32_t ident;
	uint32_t version;
	bspLump_t lumps[HEADER_LUMPS];
	uint32_t mapRevision;
};

struct bspTexData_t
{
	vec3_t reflectivity;
	uint32_t nameStringTableID;
	uint32_t width;
	uint32_t height;
	uint32_t view_width;
	uint32_t view_height;
};

struct bspTexInfo_t
{
	vec3_t textureVecS;
	float textureOffS;
	vec3_t textureVecT;
	float textureOffT;
	vec3_t lightmapVecS;
	float lightmapOffS;
	vec3_t lightmapVecT;
	float lightmapOffT;
	uint32_t flags;
	uint32_t texData;
};

struct bspFace_t
{
	uint16_t planeNum;
	uint8_t side;
	uint8_t onNode;
	int32_t firstEdge;
	uint16_t edgesCount;
	int16_t texInfo;
	int16_t dispInfo;
	int16_t surfaceFogVolumeID;

	uint8_t styles[MAXLIGHTMAPS];
	uint32_t lightOfs;
	float area;
	int32_t lightmapMins[2];
	int32_t lightmapSize[2];

	int32_t origFace;
	uint16_t numPrims;
	uint16_t firstPrimID;
	uint32_t smoothingGroups;
};

struct bspModel_t
{
	vec3_t mins;
	vec3_t maxs;
	vec3_t origin;
	int32_t headNode;
	uint32_t firstFace;
	uint32_t faceCount;
};

}//srcbsp
