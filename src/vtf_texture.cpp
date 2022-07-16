// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)

#include "texture.h"
#include "vtf.h"
#include "rgbcx.h"
#include <cstring>

int getSize(int width, int height, int depth, eVtfFormat fmt)
{
	if (fmt == eVtfFormat::BGR888 || fmt == eVtfFormat::RGB888) {
		return depth * width * height * 3;
	}
	else if (fmt == eVtfFormat::BGRA8888 || fmt == eVtfFormat::BGRX8888 || fmt == eVtfFormat::RGBA8888) {
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
		return depth * width * height * 8;
	}

	return 0;
}

// https://stackoverflow.com/a/60047308
uint32_t as_uint(const float x) {
	return *(uint32_t *)&x;
}
float as_float(const uint32_t x) {
	return *(float *)&x;
}
float half_to_float(const uint16_t x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
	const uint32_t e = (x & 0x7C00) >> 10; // exponent
	const uint32_t m = (x & 0x03FF) << 13; // mantissa
	const uint32_t v = as_uint((float)m) >> 23; // evil log2 bit hack to count leading zeros in denormalized format
	return as_float((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000))); // sign : normalized : denormalized
}
uint16_t float_to_half(const float x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
	const uint32_t b = as_uint(x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
	const uint32_t e = (b & 0x7F800000) >> 23; // exponent
	const uint32_t m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
	return (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) | ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
}

uint8_t half_to_byte(uint16_t x)
{
	int i = half_to_float(x) * 255.0f * 4.0f;
	return (i > 255) ? 255 : ((i < 0) ? 0 : i);
}

bool LoadVtfTexture(const uint8_t *data, size_t size, Texture &tex, bool scan)
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
	if (hdr.imageFormat == eVtfFormat::BGR888 || hdr.imageFormat == eVtfFormat::RGB888){
		faceSize = hdr.width * hdr.height * 3;
		fmt = Texture::RGB8;
	}
	else if (hdr.imageFormat == eVtfFormat::BGRA8888 || hdr.imageFormat == eVtfFormat::BGRX8888 || hdr.imageFormat == eVtfFormat::RGBA8888) {
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
	else if (hdr.imageFormat == eVtfFormat::RGBA16161616F) {
		faceSize = hdr.width * hdr.height * 8;
		fmt = Texture::RGBA8;
	}
	else {
		fprintf(stderr, "Error: unsupported vtf format %d\n", static_cast<int32_t>(hdr.imageFormat));
		return false;
	}

	int numFaces = 1;
	if (hdr.flags & (uint32_t)eVtfFlags::CUBEMAP)
		numFaces = (baseHdr.version[1] < 1 || baseHdr.version[1] >= 5) ? 6 : 7;

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
		if (size >= offset + faceSize * hdr.depth * hdr.numFrames * 6)
		{
			numFaces = 6;
			printf("vtf: have 6 faces instead of 7 (%s)\n",tex.name.c_str());
		}
		else
		{
			fprintf(stderr, "Error: unexpected vtf file size (%d < %d)\n", (int)size, offset + faceSize * hdr.depth * hdr.numFrames * numFaces);
			return false;
		}
	}

	if (scan)
		return true;

	tex.create(hdr.width, hdr.height, fmt);
	if (hdr.imageFormat == eVtfFormat::BGR888) {
		memcpy(&tex.data[0], data + offset, faceSize);
		if (hdr.imageFormat != eVtfFormat::RGB888)
		{
			uint8_t *td = &tex.data[0];
			for (int i = 0; i < hdr.width * hdr.height; i++)
			{
				uint8_t t = td[0];
				td[0] = td[2];
				td[2] = t;
				td += 3;
			}
		}
	}
	else if (hdr.imageFormat == eVtfFormat::BGRA8888 || hdr.imageFormat == eVtfFormat::BGRX8888)
	{
		memcpy(&tex.data[0], data + offset, faceSize);
		if (hdr.imageFormat != eVtfFormat::RGBA8888)
		{
			uint8_t *td = &tex.data[0];
			for (int i = 0; i < hdr.width * hdr.height; i++)
			{
				uint8_t t = td[0];
				td[0] = td[2];
				td[2] = t;
				if (hdr.imageFormat == eVtfFormat::BGRX8888)
					td[3] = 255;
				td += 4;
			}
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
	else if (hdr.imageFormat == eVtfFormat::RGBA16161616F) {
		const uint16_t *ts = (const uint16_t*)(data + offset);
		uint8_t *td = &tex.data[0];
		for (int i = 0; i < hdr.width * hdr.height; i++)
		{
			td[0] = half_to_byte(ts[0]);
			td[1] = half_to_byte(ts[1]);
			td[2] = half_to_byte(ts[2]);
			td[3] = half_to_byte(ts[3]);
			ts += 4;
			td += 4;
		}
	}

	return true;
}
