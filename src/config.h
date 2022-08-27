// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <string>

struct LoadConfig
{
	std::string gamePath;

	bool skipSky = false;
	int lightmapSize = 2048;
	int lstyle = -1;
	bool lstylesMerge = false;
	bool lstylesAll = false;
	bool uint16Inds = false;
	bool allTextures = false;

	bool verbose = false;
	bool scan = false;
};
