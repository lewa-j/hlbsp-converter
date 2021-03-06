// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <string>
class Map;

namespace gltf
{
	enum
	{
		UNSIGNED_SHORT = 0x1403,
		UNSIGNED_INT = 0x1405,
		FLOAT = 0x1406,

		ARRAY_BUFFER = 0x8892,
		ELEMENT_ARRAY_BUFFER = 0x8893
	};

	bool exportMap(const std::string &name, Map &map, bool verbose);
}
