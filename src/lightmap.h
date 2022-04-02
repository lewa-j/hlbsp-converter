// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include "texture.h"
#include <string>

class Lightmap
{
public:
	struct RectI
	{
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
	};

	Lightmap(int size, bool vecs = false, bool rgbexp_ = false) : 
		block_width(size), block_height(size), haveVecs(vecs), rgbexp(rgbexp_){}
	void initBlock();
	bool allocBlock(RectI &rectInOut);
	void uploadBlock(const std::string &name);

	void write(const RectI &rect, uint8_t *data, uint8_t *dataVecs = nullptr);

	bool pack(std::vector<RectI> &rects, int max_size);

	int block_width = 1024;
	int block_height = 1024;
	bool haveVecs = false;
	bool rgbexp = false;
	std::vector<int> allocated;
	int current_lightmap_texture = 0;
	Texture buffer;
	Texture bufferVecs;
};
