// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include "texture.h"
#include <string>

class Lightmap
{
public:
	Lightmap(int size, bool vecs = false) : 
		block_size(size), haveVecs(vecs), allocated(block_size){}
	void initBlock();
	bool allocBlock(int w, int h, int &x, int &y);
	void uploadBlock(const std::string &name);

	void write(int w, int h, int x, int y, uint8_t *data, uint8_t *dataVecs = nullptr);

	int block_size = 1024;
	bool haveVecs = false;
	std::vector<int> allocated;
	int current_lightmap_texture = 0;
	Texture buffer;
	Texture bufferVecs;
};
