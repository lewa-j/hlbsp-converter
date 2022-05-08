// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "bsp-converter.h"
#include "map.h"
#include "gltf_export.h"
#include "wad.h"
#include "texture.h"
#include <direct.h>
#include "rgbcx.h"

#if _MSC_VER
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#define strcasestr StrStrIA
#endif

int main(int argc, const char *argv[])
{
	printf(HLBSP_CONVERTER_NAME "\n");
	if (argc < 2 || !strcmp(argv[1], "-h"))
	{
		printf("Usage: bsp-converter map.bsp [-lm <max lightmap atlas size>] [-lstyles <light style index>|all|merge] [-skip_sky] [-uint16] [-tex] [-v]\n");
		return -1;
	}

	rgbcx::init();

	std::string fileName = argv[1];
	std::string rootPath;
	{
		int p = fileName.find_last_of("/\\");
		if (p != std::string::npos) {
			rootPath = fileName.substr(0, p + 1);
			fileName = fileName.substr(p + 1);
		}
		auto l = fileName.find_last_of('.');
		if (l)
			fileName = fileName.substr(0, l);
	}

	Map::LoadConfig config;

	for (int i = 2; i < argc; i++)
	{
		if (!strcmp(argv[i], "-game"))
		{
			if (argc > i + 1)
			{
				i++;
				config.gamePath = argv[i];
			}
			else
			{
				printf("Warning: '-game' parameter requires a path\n");
			}
		}
		else if (!strcmp(argv[i], "-lm"))
		{
			if (argc > i + 1)
			{
				i++;
				config.lightmapSize = atoi(argv[i]);
			}
			else
			{
				printf("Warning: '-lm' parameter requires a number - maximum lightmap atlas resolution\n");
			}
		}
		else if (!strcmp(argv[i], "-lstyle"))
		{
			if (argc > i + 1)
			{
				i++;
				if (!strcmp(argv[i], "all"))
					config.lstylesAll = true;
				else if (!strcmp(argv[i], "merge"))
					config.lstylesMerge = true;
				else
					config.lstyle = atoi(argv[i]);
			}
			else
			{
				printf("Warning: '-lstyle' parameter requires a number - light style index, or a word 'all' or 'merge'\n");
			}
		}
		else if (!strcmp(argv[i], "-skip_sky"))
		{
			config.skipSky = true;
		}
		else if (!strcmp(argv[i], "-uint16"))
		{
			config.uint16Inds = true;
		}
		else if (!strcmp(argv[i], "-tex"))
		{
			config.allTextures = true;
		}
		else if (!strcmp(argv[i], "-v"))
		{
			config.verbose = true;
		}
		else
		{
			printf("Warning: unknown parameter \"%s\"\n", argv[i]);
		}
	}

	if (config.verbose)
	{
		if (config.skipSky)
			printf("Sky polygons will be excluded from export\n");
		if (config.uint16Inds)
			printf("Set indices type to uint16\n");
		if (config.allTextures)
			printf("All textures will be exported\n");
	}

	if (!config.lightmapSize)
		config.lightmapSize = 2048;

	if (config.gamePath.size() && config.gamePath.back() != '/' && config.gamePath.back() != '\\')
	{
		config.gamePath += '/';
	}

	std::string mapPath;
	if (strcasestr(argv[1], ".wad") != nullptr)
	{
		WadFile wad;
		if (!wad.Load(argv[1]))
			return -1;

		_mkdir("textures");

		std::vector<uint8_t> data;
		Texture tex;
		for (int i = 0; i < wad.lumps.size(); i++)
		{
			if (wad.lumps[i].type != WadFile::TYP_MIPTEX && wad.lumps[i].type != WadFile::TYP_GFXPIC)
			{
				printf("lump %d (%s) unknown type %d\n", i, wad.lumps[i].name, wad.lumps[i].type);
				continue;
			}
			wad.GetLump(i, data);
			if (wad.lumps[i].type == WadFile::TYP_GFXPIC)
				tex.name = wad.lumps[i].name;
			if (LoadMipTexture(&data[0], tex, wad.lumps[i].type))
				tex.save((std::string("textures/") + tex.name + ".png").c_str(), config.verbose);
		}

		return 0;
	}
	else if (strcasestr(argv[1], ".vtf") != nullptr)
	{
		FILE *f = fopen(argv[1], "rb");
		if (!f)
		{
			fprintf(stderr, "Error: can't open %s: %s\n", argv[1], strerror(errno));
			return -1;
		}

		fseek(f, 0, SEEK_END);
		std::vector<uint8_t> data(ftell(f));
		fseek(f, 0, SEEK_SET);
		fread(&data[0], data.size(), 1, f);

		Texture tex;
		if (!LoadVtfTexture(data.data(), data.size(), tex))
		{
			fprintf(stderr, "Error: LoadVtfTexture %s failed\n", argv[1]);
			return -1;
		}
		tex.save((fileName + ".png").c_str(), config.verbose);

		return 0;
	}
	else if (strcasestr(argv[1], ".bsp") != nullptr)
	{
		mapPath = argv[1];
	}
	else
	{
		if (config.gamePath.size())
			mapPath = config.gamePath;

		mapPath += "maps/";
		mapPath += argv[1];
		mapPath += ".bsp";
	}

	Map map;
	if (!map.load(mapPath.c_str(), fileName.c_str(), &config))
	{
		fprintf(stderr, "Can't load map\n");
		return -1;
	}

	if (!gltf::ExportMap(fileName, map, config.verbose))
	{
		fprintf(stderr, "Export failed\n");
		return -1;
	}
	if (config.verbose)
		printf("Success");
	return 0;
}
