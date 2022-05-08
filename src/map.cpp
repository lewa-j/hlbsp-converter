// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)

#include "map.h"

bool Map::load(const char *path, const char *name, LoadConfig *config)
{
	static LoadConfig defaultConfig;
	if (!config)
		config = &defaultConfig;

	struct stat s;
	if (stat(path, &s))
	{
		fprintf(stderr, "Error: file %s doesn't exists (%s)\n", path, strerror(errno));
		return false;
	}

	if (s.st_size < 4)
	{
		fprintf(stderr, "Error: file %s is empty\n", path);
		return false;
	}

	FILE *f = fopen(path, "rb");
	if (!f)
	{
		fprintf(stderr, "Error: can't open %s: %s\n", path, strerror(errno));
		return false;
	}
	if (config->verbose)
		printf("Reading %s\n", path);

	uint32_t ident = 0;
	fread(&ident, sizeof(ident), 1, f);
	fseek(f, 0, SEEK_SET);

	bool r = false;
	switch (ident)
	{
	case HLBSP_VERSION:
	case XTBSP_VERSION:
		return load_hlbsp(f, name, config);
	case VBSP_IDENT:
		return load_vbsp(f, name, config);
	default:
		fprintf(stderr, "Error: unknown bsp version %d (%c%c%c%c)\n", ident, (char)ident, char(ident>>8), char(ident >> 16), char(ident >> 24));
		return false;
	}

	return false;
}
