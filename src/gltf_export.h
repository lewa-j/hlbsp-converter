// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include "hlbsp.h"

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

	bool ExportMap(const std::string &name, hlbsp::Map &map);
}
