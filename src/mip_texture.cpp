// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)

#include "texture.h"
#include "hlbsp.h"
#include "wad.h"
#include <cstring>

bool LoadMipTexture(const uint8_t *data, Texture &tex, int type)
{
	hlbsp::mip_t texHeader;
	bool hasAlpha = false;
	int palOffset = 0;
	if (type == WadFile::TYP_MIPTEX)
	{
		memcpy(&texHeader, data, sizeof(texHeader));

		if (texHeader.offsets[0] == 0)
			return false;

		tex.name = texHeader.name;
		hasAlpha = strchr(texHeader.name, '{') != nullptr;
		palOffset = ((texHeader.width * texHeader.height * 85) >> 6) + sizeof(uint16_t);
	}
	else if (type == WadFile::TYP_GFXPIC)
	{
		memcpy(&texHeader.width, data, sizeof(int));
		memcpy(&texHeader.height, data + 4, sizeof(int));
		texHeader.offsets[0] = 8;
		palOffset = texHeader.width * texHeader.height + sizeof(uint16_t);
	}
	else
	{
		return false;
	}

	const uint8_t *ids = data + texHeader.offsets[0];
	// two bytes before palette is a count of colors but it is always 256
	const uint8_t *pal = ids + palOffset;

	if (type == WadFile::TYP_GFXPIC && pal[255 * 3] == 0 && pal[255 * 3 + 1] == 0 && pal[255 * 3 + 2] == 255)
		hasAlpha = true;

	tex.create(texHeader.width, texHeader.height, hasAlpha ? Texture::RGBA8 : Texture::RGB8);
	uint8_t *dst = &tex.data[0];

	// decode 8bit paletted texture into 24bit rgb or 32bit rgba
	int len = texHeader.width * texHeader.height;
	for (int j = 0; j < len; j++)
	{
		int col = ids[j] * 3;
		dst[0] = pal[col + 0];
		dst[1] = pal[col + 1];
		dst[2] = pal[col + 2];
		if (hasAlpha)
		{
			if (col == (255 * 3))
				dst[0] = dst[1] = dst[2] = dst[3] = 0;
			else
				dst[3] = 255;
			dst += 4;
		}
		else
			dst += 3;
	}

	return true;
}
