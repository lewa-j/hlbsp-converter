// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <stdint.h>
#include <vector>
#include <string>

class Texture
{
public:
	enum Format
	{
		RGB8,
		RGBA8
	};

	Texture();
	Texture(int w, int h, Format fmt);

	void create(int w, int h, Format fmt);
	void clearColor();
	void get(int x, int y, uint8_t *color);
	uint8_t *get(int x, int y);
	void set(int x, int y, uint8_t *color);
	bool save(const char *path, bool verbose);

	int width = 0;
	int height = 0;
	Format format = RGBA8;
	std::vector<uint8_t> data;
	std::string name;
};

bool LoadMipTexture(const uint8_t *data, Texture &tex, int type = 67);
bool LoadVtfTexture(const uint8_t *data, size_t size, Texture &tex);
