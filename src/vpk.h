// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <string>

struct vpkHeader1_t
{
	uint32_t magic;
	uint32_t version;
	uint32_t treeSize;
};

struct vpkHeader2_t
{
	uint32_t fileDataSectionSize;
	uint32_t archiveMD5SectionSize;
	uint32_t otherMD5SectionSize;
	uint32_t signatureSectionSize;
};

struct vpkEntryHeader_t
{
	uint32_t crc;
	uint16_t smallDataSize;
	uint16_t archiveIndex;
	uint32_t offset;
	uint32_t length;
};

class VpkFile
{
public:
	bool load(const char *path);
	bool getFile(const char *path, std::vector<uint8_t> &out_data);

	std::string filePath;
	std::string basePath;
	int dataOffset;
	std::vector<uint8_t> treeData;
	std::unordered_map<std::string, const vpkEntryHeader_t *> entries;
};
