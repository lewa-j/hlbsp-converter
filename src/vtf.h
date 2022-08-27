// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <stdint.h>
#include "vector_math.h"

enum class eVtfFormat : int32_t
{
	UNKNOWN = -1,
	RGBA8888 = 0,
	ABGR8888,
	RGB888,
	BGR888,
	RGB565,
	I8,
	IA88,
	P8,
	A8,
	RGB888_BLUESCREEN,
	BGR888_BLUESCREEN,
	ARGB8888,
	BGRA8888,
	DXT1,
	DXT3,
	DXT5,
	BGRX8888,
	BGR565,
	BGRX5551,
	BGRA4444,
	DXT1_ONEBITALPHA,
	BGRA5551,
	UV88,
	UVWQ8888,
	RGBA16161616F,
	RGBA16161616,
	UVLX8888,
	R32F,
	RGB323232F,
	RGBA32323232F,
	RG1616F,
	RG3232F,
	RGBX8888,
	DUMMY,
	ATI2N,
	ATI1N,
	RGBA1010102,
	BGRA1010102,
	R16F,
	D16,
	D15S1,
	D32,
	D24S8,
	LINEAR_D24S8,
	D24X8,
	D24X4S4,
	D24FS8,
	D16_SHADOW,
	D24X8_SHADOW,
	LINEAR_BGRX8888,
	LINEAR_RGBA8888,
	LINEAR_ABGR8888,
	LINEAR_ARGB8888,
	LINEAR_BGRA8888,
	LINEAR_RGB888,
	LINEAR_BGR888,
	LINEAR_BGRX5551,
	LINEAR_I8,
	LINEAR_RGBA16161616,
	LE_BGRX8888,
	LE_BGRA8888,
	COUNT
};

const char *vtfFormatToStr(eVtfFormat fmt);

enum class eVtfFlags
{
	CUBEMAP = 0x4000
};

#pragma pack(1)
struct vtfHdrBase_t
{
	char ident[4];
	int32_t version[2];
	int32_t headerLength;
};

struct vtfHdr_7_1_t
{
	uint16_t width;
	uint16_t height;
	uint32_t flags;
	uint16_t numFrames;
	uint16_t startFrame;
	uint32_t pad1;
	vec3_t	 treflectivity;
	uint32_t pad2;
	float	 bumpScale;
	eVtfFormat imageFormat;
	uint8_t numMipLevels;
	eVtfFormat lowResImageFormat;
	uint8_t lowResImageWidth;
	uint8_t lowResImageHeight;
};

struct vtfHdr_7_2_t : public vtfHdr_7_1_t
{
	uint16_t depth;
};

struct vtfHdr_7_3_t : public vtfHdr_7_2_t
{
	char pad4[3];
	uint32_t numResources;
	uint32_t pad5[2];
};
#pragma pack()

