// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "texture.h"

#ifdef _WIN32
#include <direct.h>
#elif __linux__
#include <sys/stat.h>
#define _mkdir(path) mkdir(path, 0755)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Texture::Texture()
{
}

Texture::Texture(int w, int h, Format fmt)
{
	create(w, h, fmt);
}

void Texture::create(int w, int h, Format fmt)
{
	width = w;
	height = h;
	format = fmt;
	int bpp = (format == RGB8 ? 3 : 4);
	data.resize(w * h * bpp);
}

void Texture::clearColor()
{
	if (data.size())
		memset(&data[0], 0, data.size());
}

void Texture::get(int x, int y, uint8_t *color)
{
	int bpp = (format == RGB8 ? 3 : 4);
	int offs = (y * width + x) * bpp;
	memcpy(color, &data[offs], bpp);
}

uint8_t *Texture::get(int x, int y)
{
	int bpp = (format == RGB8 ? 3 : 4);
	int offs = (y * width + x) * bpp;
	return &data[offs];
}

void Texture::set(int x, int y, uint8_t *color)
{
	int bpp = (format == RGB8 ? 3 : 4);
	int offs = (y * width + x) * bpp;
	memcpy(&data[offs], color, bpp);
}

void createDirs(std::string path)
{
	size_t p = 0;
	while (true)
	{
		p = path.find('/', p + 1);
		if (p == std::string::npos)
			return;

		_mkdir(path.substr(0, p).c_str());
	}
}

bool Texture::save(const char *path, bool verbose)
{
	createDirs(path);

	int bpp = (format == RGB8 ? 3 : 4);
	int r = stbi_write_png(path, width, height, bpp, data.data(), bpp * width);
	if(verbose)
		printf("Writing: %s \t%s\n", path, r ? "success" : "failed");
	return !!r;
}
