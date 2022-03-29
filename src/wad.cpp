// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "wad.h"

WadFile::~WadFile()
{
	if(f)
		fclose(f);
}

bool WadFile::Load(const char *path)
{
	f = fopen(path, "rb");
	if (!f)
	{
		fprintf(stderr, "Error: can't open %s: %s\n", path, strerror(errno));
		return false;
	}

	header_t header;
	fread(&header, sizeof(header), 1, f);

	if (header.ident != IDWAD3HEADER)
	{
		fprintf(stderr, "Error: unknown ident %d (%c%c%c%c) in %s\n", header.ident, 
			(char)header.ident, char(header.ident >> 8), char(header.ident >> 16), char(header.ident >> 24), path);
		return false;
	}

	lumps.resize(header.numlumps);
	if (header.numlumps)
	{
		fseek(f, header.infotableofs, SEEK_SET);
		fread(&lumps[0], sizeof(lumps[0]), lumps.size(), f);
	}

	printf("Loaded %s with %d lumps\n", path, lumps.size());

	return true;
}

bool WadFile::GetLump(int index, std::vector<uint8_t> &data)
{
	if (index < 0 || index >= lumps.size())
		return false;

	data.resize(lumps[index].disksize);
	fseek(f, lumps[index].filepos, SEEK_SET);
	fread(&data[0], sizeof(data[0]), data.size(), f);

	return true;
}

bool WadFile::FindLump(const char *name, std::vector<uint8_t> &data)
{
	int i = 0;
	for (; i < lumps.size(); i++)
	{
		if (!strnicmp(name, lumps[i].name, 16))
			break;
	}
	if (i == lumps.size())
		return false;

	return GetLump(i, data);
}
