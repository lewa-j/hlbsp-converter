// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "vpk.h"
#include <stdio.h>
#include <cstring>

bool VpkFile::load(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (!file) {
		fprintf(stderr, "Error: Can't open %s: %s\n", path, strerror(errno));
		return false;
	}

	filePath = path;
	size_t sf = filePath.find("_dir.vpk");
	if (sf != std::string::npos) {
		basePath = filePath.substr(0, sf + 1);
	}

	vpkHeader1_t header{};
	fread(&header, sizeof(header), 1, file);
	dataOffset = sizeof(header) + header.treeSize;
	vpkHeader2_t header2{};
	if (header.version == 2) {
		fread(&header2, sizeof(header2), 1, file);
		dataOffset += sizeof(header2);
	}

	treeData.resize(header.treeSize);
	fread(treeData.data(), header.treeSize, 1, file);
	const char *r = (const char *)treeData.data();
	const char *rend = (const char *)treeData.data() + treeData.size();
	while (*r) {
		const char *ext = r;
		r += strlen(r) + 1;
		while (*r) {
			const char *dir = r;
			r += strlen(r) + 1;
			while (*r) {
				const char *name = r;
				std::string entryName = std::string(dir) + "/";
				if (entryName == " /")
					entryName.clear();
				entryName += std::string(name) + "." + ext;
				r += strlen(r) + 1;
				const vpkEntryHeader_t &entry = *(const vpkEntryHeader_t *)r;
				r += sizeof(vpkEntryHeader_t);
				r += 2;//0xffff terminator
				r += entry.smallDataSize;
				entries[entryName] = &entry;
				if (r >= rend)
					break;
			}
			if (r >= rend)
				break;
			r++;
		}
		if (r >= rend)
			break;
		r++;
	}

	fclose(file);

	return true;
}

bool VpkFile::getFile(const char *path, std::vector<uint8_t> &out_data)
{
	auto fe = entries.find(std::string(path));
	if (fe == entries.end())
		return false;

	const vpkEntryHeader_t *ent = fe->second;
	if (ent->archiveIndex == 0x7FFF)
	{
		FILE *file = fopen(filePath.c_str(), "rb");
		if (!file)
			return false;
		fseek(file, dataOffset + ent->offset, SEEK_SET);
		out_data.resize(ent->length);
		fread(&out_data[0], ent->length, 1, file);
		fclose(file);
		return true;
	}
	if (basePath.empty())
		return false;
	char newPath[2048]{};
	int n = snprintf(newPath, 2047, "%s%.3d.vpk", basePath.c_str(), ent->archiveIndex);
	if (n < 0 || n >= 2048)
		return false;
	newPath[n] = 0;

	FILE *file = fopen(newPath, "rb");
	if (!file)
		return false;
	fseek(file, ent->offset, SEEK_SET);
	out_data.resize(ent->length);
	fread(&out_data[0], ent->length, 1, file);
	fclose(file);

	return true;
}
