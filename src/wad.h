// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <vector>

class WadFile
{
public:
	enum
	{
		IDWAD3HEADER = (('3'<<24)+('D'<<16)+('A'<<8)+'W')	// little-endian "WAD3" half-life wads
	};

	~WadFile();

	struct header_t
	{
		int32_t	ident;
		int32_t	numlumps;
		int32_t	infotableofs;
	};

	struct lumpinfo_t
	{
		int32_t	filepos;
		int32_t	disksize;	// compressed or uncompressed
		int32_t	size;		// uncompressed
		uint8_t	type;
		uint8_t	attribs;
		uint8_t	imgType;
		uint8_t	pad;
		char	name[16];
	};

	FILE *f = nullptr;
	std::vector<lumpinfo_t> lumps;

	bool Load(const char *path);
	bool GetLump(const char *name, std::vector<uint8_t> &data);
};
