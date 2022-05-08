// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "lightmap.h"

// atlas packing code from https://github.com/id-Software/Quake-2/blob/master/ref_gl/gl_rsurf.c

void Lightmap::initBlock()
{
	if (allocated.size() != block_width)
		allocated.resize(block_width);
	memset(allocated.data(), 0, allocated.size() * sizeof(allocated[0]));
	buffer.create(block_width, block_height, Texture::RGB8);
	if(haveVecs)
		bufferVecs.create(block_width, block_height, Texture::RGB8);
}

bool Lightmap::allocBlock(RectI &rect)
{
	int	i, j;
	int	best, best2;

	best = block_height;

	for (i = 0; i < block_width - rect.w; i++)
	{
		best2 = 0;

		for (j = 0; j < rect.w; j++)
		{
			if (allocated[i + j] >= best)
				break;
			if (allocated[i + j] > best2)
				best2 = allocated[i + j];
		}

		if (j == rect.w)
		{
			// this is a valid spot
			rect.x = i;
			rect.y = best = best2;
		}
	}

	if (best + rect.h > block_height)
		return false;

	for (i = 0; i < rect.w; i++)
		allocated[rect.x + i] = best + rect.h;

	return true;
}

void Lightmap::uploadBlock(const std::string &name, bool verbose)
{
	buffer.save((name + "_lightmap" + std::to_string(current_lightmap_texture) + ".png").c_str(), verbose);
	buffer.clearColor();
	if (haveVecs)
	{
		bufferVecs.save((name + "_deluxemap" + std::to_string(current_lightmap_texture) + ".png").c_str(), verbose);
		bufferVecs.clearColor();
	}
	current_lightmap_texture++;
}

void Lightmap::write(const RectI &rect, uint8_t *data, uint8_t *dataVecs)
{
	uint8_t *dst = buffer.get(rect.x, rect.y);
	if (rgbexp)
	{
		for (int i = 0; i < rect.h; i++)
		{
			for (int j = 0; j < rect.w; j++)
			{
				float e = powf(2.0f, ((int8_t *)data)[3]);
				float r = pow(data[0] * e / 255.0f, 1.0 / 2.2) * 0.5f;
				float g = pow(data[1] * e / 255.0f, 1.0 / 2.2) * 0.5f;
				float b = pow(data[2] * e / 255.0f, 1.0 / 2.2) * 0.5f;
				float mc = fmax(r, fmax(g, b));
				if (mc > 1) {
					r /= mc;
					g /= mc;
					b /= mc;
				}
				dst[0] = uint8_t(r * 255);
				dst[1] = uint8_t(g * 255);
				dst[2] = uint8_t(b * 255);

				dst += 3;
				data += 4;
			}
			dst += (buffer.width - rect.w) * 3;
		}
	}
	else
	{
		for (int i = 0; i < rect.h; i++)
		{
			memcpy(dst, data, rect.w * 3);
			dst += buffer.width * 3;
			data += rect.w * 3;
		}
	}

	if (!haveVecs || !dataVecs)
		return;

	dst = bufferVecs.get(rect.x, rect.y);
	for (int i = 0; i < rect.h; i++)
	{
		memcpy(dst, dataVecs, rect.w * 3);
		dst += bufferVecs.width * 3;
		dataVecs += rect.w * 3;
	}
}

bool Lightmap::pack(std::vector<RectI> &rects, int max_size)
{
	block_width = block_height = 32;

	while (true)
	{
		allocated.resize(block_width);
		memset(allocated.data(), 0, allocated.size() * sizeof(allocated[0]));

		size_t i = 0;
		for (; i < rects.size(); i++)
		{
			if (rects[i].w * rects[i].h == 0)
				continue;
			if (!allocBlock(rects[i]))
				break;
		}

		if (i == rects.size())
			break;

		if (block_height > block_width)
			block_width <<= 1;
		else
			block_height <<= 1;

		if (block_width > max_size || block_height > max_size)
		{
			allocated.clear();
			return false;
		}
	}

	return true;
}
