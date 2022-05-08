// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)

#include "texture.h"
#include "vtf.h"
#include "rgbcx.h"

int getSize(int width, int height, int depth, eVtfFormat fmt)
{
	if (fmt == eVtfFormat::BGR888) {
		return depth * width * height * 3;
	}
	else if (fmt == eVtfFormat::BGRA8888) {
		return depth * width * height * 4;
	}
	else if (fmt == eVtfFormat::DXT1) {
		return depth * ((width + 3) / 4) * ((height + 3) / 4) * 8;
	}
	else if (fmt == eVtfFormat::DXT5) {
		return depth * ((width + 3) / 4) * ((height + 3) / 4) * 16;
	}
	else if (fmt == eVtfFormat::UV88) {
		return depth * width * height * 2;
	}
	else if (fmt == eVtfFormat::RGBA16161616F) {
		return depth * width * height * 4;
	}

	return 0;
}

bool LoadVtfTexture(const uint8_t *data, size_t size, Texture &tex)
{
	vtfHdrBase_t baseHdr{};
	vtfHdr_7_3_t hdr{};

	if (size < sizeof(vtfHdrBase_t))
		return false;
	memcpy(&baseHdr, data, sizeof(baseHdr));

	if (strncmp(baseHdr.ident, "VTF", 4))
	{
		fprintf(stderr, "Error: wrong vtf ident %.4s 0x%X\n", baseHdr.ident, *(uint32_t *)baseHdr.ident);
		return false;
	}

	if (baseHdr.version[0] != 7 || baseHdr.version[1] > 5)
	{
		fprintf(stderr, "Error: unknown vtf version %d.%d\n", baseHdr.version[0], baseHdr.version[1]);
		return false;
	}

	if (size < baseHdr.headerLength)
		return false;

	hdr.depth = 1;
	hdr.numResources = 0;
	if (baseHdr.version[1] == 0 || baseHdr.version[1] == 1)
	{
		memcpy(&hdr, data + sizeof(baseHdr), sizeof(vtfHdr_7_1_t));
	}
	else if (baseHdr.version[1] == 2)
	{
		memcpy(&hdr, data + sizeof(baseHdr), sizeof(vtfHdr_7_2_t));
	}
	else if (baseHdr.version[1] >= 3)
	{
		memcpy(&hdr, data + sizeof(baseHdr), sizeof(vtfHdr_7_3_t));
	}

	int offset = baseHdr.headerLength + ((hdr.lowResImageWidth + 3) / 4) * ((hdr.lowResImageHeight + 3) / 4) * 8;
	int faceSize = 0;
	Texture::Format fmt = Texture::RGBA8;
	if (hdr.imageFormat == eVtfFormat::BGR888){
		faceSize = hdr.width * hdr.height * 3;
		fmt = Texture::RGB8;
	}
	else if (hdr.imageFormat == eVtfFormat::BGRA8888) {
		faceSize = hdr.width * hdr.height * 4;
		fmt = Texture::RGBA8;
	}
	else if (hdr.imageFormat == eVtfFormat::DXT1) {
		faceSize = ((hdr.width + 3) / 4) * ((hdr.height + 3) / 4) * 8;
		fmt = Texture::RGBA8;
	}
	else if (hdr.imageFormat == eVtfFormat::DXT5) {
		faceSize = ((hdr.width + 3) / 4) * ((hdr.height + 3) / 4) * 16;
		fmt = Texture::RGBA8;
	}
	else if (hdr.imageFormat == eVtfFormat::UV88) {
		faceSize = hdr.width * hdr.height * 2;
		fmt = Texture::RGB8;
	}
	else {
		fprintf(stderr, "Error: unsupported vtf format %d\n", hdr.imageFormat);
		return false;
	}

	int numFaces = 1;
	if (hdr.flags & (uint32_t)eVtfFlags::CUBEMAP)
		numFaces = baseHdr.version[1] >= 5 ? 6 : 7;

	{
		int w = hdr.width >> 1;
		int h = hdr.height >> 1;
		int d = hdr.depth >> 1;
		for (int i = 1; i < hdr.numMipLevels; i++)
		{
			if (w < 1) w = 1;
			if (h < 1) h = 1;
			if (d < 1) d = 1;
			offset += getSize(w, h, d, hdr.imageFormat) * hdr.numFrames * numFaces;
			w = w >> 1;
			h = h >> 1;
			d = d >> 1;
		}
	}

	if (size < offset + faceSize * hdr.depth * hdr.numFrames * numFaces)
	{
		fprintf(stderr, "Error: unexpected vtf file size (%d < %d)\n", (int)size, offset + faceSize * hdr.depth * hdr.numFrames * numFaces);
		return false;
	}

	tex.create(hdr.width, hdr.height, fmt);
	if (hdr.imageFormat == eVtfFormat::BGR888) {
		memcpy(&tex.data[0], data + offset, faceSize);
		uint8_t *td = &tex.data[0];
		for (int i = 0; i < hdr.width * hdr.height; i++)
		{
			uint8_t t = td[0];
			td[0] = td[3];
			td[3] = t;
			td += 3;
		}
	}
	else if (hdr.imageFormat == eVtfFormat::BGRA8888)
	{
		memcpy(&tex.data[0], data + offset, faceSize);
		uint8_t *td = &tex.data[0];
		for (int i = 0; i < hdr.width * hdr.height; i++)
		{
			uint8_t t = td[0];
			td[0] = td[3];
			td[3] = t;
			td += 4;
		}
	}
	else if (hdr.imageFormat == eVtfFormat::UV88) {
		const uint8_t *ts = data + offset;
		uint8_t *td = &tex.data[0];
		for (int i = 0; i < hdr.width * hdr.height; i++)
		{
			td[0] = ts[0];
			td[1] = ts[1];
			ts += 2;
			td += 4;
		}
	}
	else if (hdr.imageFormat == eVtfFormat::DXT1) {
		const uint8_t *ts = data + offset;
		std::vector<uint8_t> block(64);
		for (int i = 0; i < hdr.height; i += 4)
		{
			for (int j = 0; j < hdr.width; j += 4)
			{
				rgbcx::unpack_bc1(ts, &block[0]);
				for (int k = 0; k < 4; k++)
					memcpy(tex.get(j, i + k), &block[k * 16], 16);
				ts += 8;
			}
		}
	}
	else if (hdr.imageFormat == eVtfFormat::DXT5) {
		const uint8_t *ts = data + offset;
		std::vector<uint8_t> block(64);
		for (int i = 0; i < hdr.height; i += 4)
		{
			for (int j = 0; j < hdr.width; j += 4)
			{
				rgbcx::unpack_bc3(ts, &block[0]);
				for (int k = 0; k < 4; k++)
					memcpy(tex.get(j, i + k), &block[k * 16], 16);
				ts += 16;
			}
		}
	}

	return true;
}
