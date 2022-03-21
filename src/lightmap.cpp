// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "lightmap.h"

// atlas packing code from https://github.com/id-Software/Quake-2/blob/master/ref_gl/gl_rsurf.c

void Lightmap::initBlock()
{
	memset(allocated.data(), 0, allocated.size() * sizeof(allocated[0]));
	buffer.create(block_size, block_size, Texture::RGB8);
}

bool Lightmap::allocBlock(int w, int h, int &x, int &y)
{
	int	i, j;
	int	best, best2;

	best = block_size;

	for (i = 0; i < block_size - w; i++)
	{
		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (allocated[i + j] >= best)
				break;
			if (allocated[i + j] > best2)
				best2 = allocated[i + j];
		}

		if (j == w)
		{
			// this is a valid spot
			x = i;
			y = best = best2;
		}
	}

	if (best + h > block_size)
		return false;

	for (i = 0; i < w; i++)
		allocated[x + i] = best + h;

	return true;
}

void Lightmap::uploadBlock(const std::string &name)
{
	buffer.save((name + "_lightmap" + std::to_string(current_lightmap_texture) + ".png").c_str());
	buffer.clearColor();
	current_lightmap_texture++;
}

void Lightmap::write(int w, int h, int x, int y, uint8_t *data)
{
	uint8_t *dst = buffer.get(x, y);
	for (int i = 0; i < h; i++)
	{
		memcpy(dst, data, w * 3);
		dst += buffer.width * 3;
		data += w * 3;
	}
}
