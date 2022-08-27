// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "hlbsp.h"
#include <stdio.h>
#include <map>
#include <set>
#include "map.h"
#include "lightmap.h"
#include "wad.h"
#include "parser.h"
#include <cfloat>
#include <cstring>

#ifdef __linux__
#include <sys/stat.h>
#include <strings.h>

#define strnicmp strncasecmp
#elif _MSC_VER
#define strnicmp _strnicmp
#endif

bool Map::load_hlbsp(FILE *f, const char *name, LoadConfig *config)
{
	using namespace hlbsp;

	dheader_t header;
	dheader31_t header31;
	dextrahdr_t headerExtra;
	int lmSampleSize = 16;
	fread(&header, sizeof(header), 1, f);

	switch (header.version)
	{
	case HLBSP_VERSION:
		lmSampleSize = 16;
		break;
	case XTBSP_VERSION:
		lmSampleSize = 8;
		break;
	default:
		fprintf(stderr, "Error: unknown bsp version %d\n", header.version);
		fclose(f);
		return false;
	}

	if (header.version == XTBSP_VERSION)
		fread(&header31, sizeof(header31), 1, f);

	fread(&headerExtra, sizeof(headerExtra), 1, f);

	if (headerExtra.id != IDEXTRAHEADER || headerExtra.version != EXTRA_VERSION)
		headerExtra.id = 0; // no extra header

	std::vector<char> entitiesText;
	std::vector<dplane_t> planes;
	std::vector<vec3_t> bspVertices;
	std::vector<dmodel_t> bspModels;
	std::vector<dfaceinfo_t> faceInfos;
	std::vector<dface_t> faces;
	std::vector<int> surfedges;
	std::vector<dedge_t> edges;
	std::vector<dtexinfo_t> texinfos;
	std::vector<uint8_t> lightmapPixels;
	std::vector<uint8_t> lightmapVecs;

#define READ_LUMP(to, lump) \
	to.resize(lump.filelen / sizeof(to[0])); \
	if(lump.filelen) \
	{ \
		fseek(f, lump.fileofs, SEEK_SET); \
		fread(&to[0], lump.filelen, 1, f); \
	}

	READ_LUMP(entitiesText, header.lumps[LUMP_ENTITIES]);
	entitiesText.push_back('\0');
	READ_LUMP(planes, header.lumps[LUMP_PLANES]);
	READ_LUMP(bspVertices, header.lumps[LUMP_VERTEXES]);
	READ_LUMP(bspModels, header.lumps[LUMP_MODELS]);
	READ_LUMP(faces, header.lumps[LUMP_FACES]);
	READ_LUMP(surfedges, header.lumps[LUMP_SURFEDGES]);
	READ_LUMP(edges, header.lumps[LUMP_EDGES]);
	READ_LUMP(texinfos, header.lumps[LUMP_TEXINFO]);
	READ_LUMP(lightmapPixels, header.lumps[LUMP_LIGHTING]);
	if (headerExtra.id)
	{
		READ_LUMP(lightmapVecs, headerExtra.lumps[LUMP_LIGHTVECS]);
		READ_LUMP(faceInfos, headerExtra.lumps[LUMP_FACEINFO]);
	}
#undef READ_LUMP

	if (config->verbose)
		printf("Load %d models\n", (int)bspModels.size());
	models.resize(bspModels.size());

	parseEntities(&entitiesText[0], entitiesText.size());

	std::vector<WadFile> wads;
	if (config->allTextures)
	{
		std::string basePath;
		if (config->gamePath.size() && config->gamePath.find("valve") == std::string::npos)
		{
			size_t l = config->gamePath.find_last_of("/\\", config->gamePath.size() - 2);
			if (l != std::string::npos)
				basePath = config->gamePath.substr(0, l) + "/valve/";
		}
		wads.resize(wadNames.size());
		for (int i = 0; i < wadNames.size(); i++)
		{
			std::string wadPath = config->gamePath + wadNames[i];
			struct stat statBuffer;
			if (!stat(wadPath.c_str(), &statBuffer))
			{
				wads[i].load(wadPath.c_str());
				continue;
			}

			if (!basePath.empty())
			{
				wadPath = basePath + wadNames[i];
				if (!stat(wadPath.c_str(), &statBuffer))
				{
					wads[i].load(wadPath.c_str());
					continue;
				}
			}

			printf("Warning: %s not found\n", wadNames[i].c_str());
		}
	}

	hlbsp_loadTextures(f, header.lumps[LUMP_TEXTURES].fileofs, header.lumps[LUMP_TEXTURES].filelen, wads, config->verbose);

	fclose(f);

	if (lightmapPixels.empty() && config->verbose)
	{
		printf("Map without lightmaps\n");
	}

	materials.resize(textures.size());
	for (int i = 0; i < materials.size(); i++)
	{
		materials[i].name = textures[i].name;
		materials[i].texture = i;
		materials[i].alphaMask = (textures[i].name.find('{') != std::string::npos);
		materials[i].lightmapped = true;
	}

	std::vector<Lightmap::RectI> lmRects(faces.size());
	std::vector<vec2i_t> lmMins(faces.size());

	Lightmap lightmap(config->lightmapSize, lightmapVecs.size() != 0);

	int numedges = (int)edges.size();
	std::vector<std::map<int, std::vector<int> > > modelMaterialFaces(bspModels.size());
	for (int mi = 0; mi < bspModels.size(); mi++)
	{
		const auto &m = bspModels[mi];
		// sort faces by texture and batch into a single mesh

		for (int fi = m.firstface; fi < m.firstface + m.numfaces; fi++)
		{
			const auto &f = faces[fi];
			const auto &ti = texinfos[f.texinfo];
			auto &rect = lmRects[fi];
			rect = { 0,0,0,0 };

			if (config->skipSky && !strnicmp(textures[ti.miptex].name.data(), "sky", textures[ti.miptex].name.size()))
				continue;
			modelMaterialFaces[mi][ti.miptex].push_back(fi);

			// lightmap calculations
			if (lightmapPixels.empty() || f.lightofs == -1 || f.styles[0] == 255)
				continue;

			int sampleSize = lmSampleSize;
			if (ti.faceInfo >= 0 && ti.faceInfo < faceInfos.size())
				sampleSize = faceInfos[ti.faceInfo].textureStep;

			vec2_t min_uv{ FLT_MAX, FLT_MAX };
			vec2_t max_uv{ -FLT_MAX, -FLT_MAX };
			for (int j = 0; j < f.numedges; j++)
			{
				int e = surfedges[f.firstedge + j];
				if (e >= numedges || e <= -numedges)
				{
					fprintf(stderr, "Error: model %d face %d bad edge %d\n", mi, fi, e);
					return false;
				}
				int vi = edges[abs(e)].v[(e > 0 ? 0 : 1)];
				if (vi >= bspVertices.size()) {
					fprintf(stderr, "Error: model %d face %d bad vertex index %d\n", mi, fi, vi);
					return false;
				}
				const vec3_t &v = bspVertices[vi];
				vec2_t uv = { static_cast<float>(v.dot_d(ti.texVecS)) + ti.texOffS, static_cast<float>(v.dot(ti.texVecT)) + ti.texOffT };
				min_uv.x = fminf(min_uv.x, uv.x);
				max_uv.x = fmaxf(max_uv.x, uv.x);
				min_uv.y = fminf(min_uv.y, uv.y);
				max_uv.y = fmaxf(max_uv.y, uv.y);
			}

			lmMins[fi] = { int(floor(min_uv.x / sampleSize)), int(floor(min_uv.y / sampleSize)) };
			rect.w = ceil(max_uv.x / sampleSize) - lmMins[fi].x + 1;
			rect.h = ceil(max_uv.y / sampleSize) - lmMins[fi].y + 1;
		}
	}

	if (lightmapPixels.size())
	{
		lightmap.pack(lmRects, config->lightmapSize);

		lightmap.initBlock();
	}

	int indicesOffset = 0;
	for (int mi = 0; mi < bspModels.size(); mi++)
	{
		const auto &m = bspModels[mi];

		models[mi].meshes.push_back({});
		mesh_t *mesh = &models[mi].meshes.back();

		mesh->count = 0;
		mesh->offset = indicesOffset;
		mesh->vertOffset = (int)vertices.size();
		int indVertOffset = 0;
		for (auto &mat : modelMaterialFaces[mi])
		{
			submesh_t submesh;
			submesh.material = mat.first;
			submesh.offset = indicesOffset;
			submesh.count = 0;

			for (int i = 0; i < mat.second.size(); i++)
			{
				const auto &f = faces[mat.second[i]];
				const auto &ti = texinfos[f.texinfo];
				const auto &plane = planes[f.planenum];

				if (config->uint16Inds && (indVertOffset + f.numedges >= UINT16_MAX - 1))
				{
					mesh->vertCount = (int)vertices.size() - mesh->vertOffset;
					if (submesh.count)
					{
						indicesOffset += submesh.count;
						mesh->count += submesh.count;
						mesh->submeshes.push_back(submesh);
					}

					models[mi].meshes.push_back({});
					mesh = &models[mi].meshes.back();

					mesh->count = 0;
					mesh->offset = indicesOffset;
					mesh->vertOffset = (int)vertices.size();
					indVertOffset = 0;

					submesh.material = mat.first;
					submesh.offset = indicesOffset;
					submesh.count = 0;
				}

				size_t faceVertOffset = vertices.size();

				for (int j = 0; j < f.numedges; j++)
				{
					int e = surfedges[f.firstedge + j];
					int vi = edges[abs(e)].v[(e > 0 ? 0 : 1)];
					vert_t v{ bspVertices[vi], plane.normal };
					if (f.side)
					{
						v.norm.x = -v.norm.x;
						v.norm.y = -v.norm.y;
						v.norm.z = -v.norm.z;
					}
					v.uv = { v.pos.dot(ti.texVecS) + ti.texOffS, v.pos.dot(ti.texVecT) + ti.texOffT };
					vertices.push_back(v);
				}

				if (lightmapPixels.size() && f.lightofs != -1)
				{
					auto &rect = lmRects[mat.second[i]];
					int sampleSize = lmSampleSize;
					if (ti.faceInfo >= 0 && ti.faceInfo < faceInfos.size())
						sampleSize = faceInfos[ti.faceInfo].textureStep;

					vec2i_t mins = lmMins[mat.second[i]];
					if (f.styles[0] == 0)
						lightmap.write(rect, &lightmapPixels[f.lightofs], lightmapVecs.size() ? &lightmapVecs[f.lightofs] : nullptr);

					for (int j = 0; j < f.numedges; j++)
					{
						vert_t &v = vertices[faceVertOffset + j];
						if (f.styles[0] != 255)
						{
							v.uv2.x = (v.uv.x - mins.x * sampleSize + rect.x * sampleSize + sampleSize * 0.5f) / (lightmap.block_width * sampleSize);
							v.uv2.y = (v.uv.y - mins.y * sampleSize + rect.y * sampleSize + sampleSize * 0.5f) / (lightmap.block_height * sampleSize);
						}
						else
						{
							v.uv2 = { 1.0f - (0.5f / lightmap.block_width), 1.0f - (0.5f / lightmap.block_height) };
						}
					}
				}

				const Texture &tex = textures[ti.miptex];
				if (tex.width && tex.height)
				{
					for (int j = 0; j < f.numedges; j++)
					{
						vert_t &v = vertices[faceVertOffset + j];
						v.uv.x /= tex.width;
						v.uv.y /= tex.height;
					}
				}

				// turn TRIANGLE_FAN into TRIANGLES
				for (int j = 1; j < f.numedges - 1; j++)
				{
					if (config->uint16Inds)
					{
						indices16.push_back(indVertOffset);
						indices16.push_back(indVertOffset + j + 1);
						indices16.push_back(indVertOffset + j);
					}
					else
					{
						indices32.push_back(indVertOffset);
						indices32.push_back(indVertOffset + j + 1);
						indices32.push_back(indVertOffset + j);
					}
				}
				submesh.count += (f.numedges - 2) * 3;
				indVertOffset += f.numedges;
			}
			if (submesh.count)
				mesh->submeshes.push_back(submesh);

			mesh->count += submesh.count;
			indicesOffset += submesh.count;
		}
		mesh->vertCount = (int)vertices.size() - mesh->vertOffset;
	}

	if (lightmapPixels.size())
	{
		lightmap.uploadBlock(name, config->verbose);

		std::set<int> lstyles;
		for (int i = 0; i < faces.size(); i++)
		{
			const dface_t &f = faces[i];
			for (int j = 0; j < LM_STYLES && f.styles[j] != 255; j++)
			{
				if (f.styles[j] != 0 && (config->lstylesAll || config->lstylesMerge || f.styles[j] == config->lstyle))
				{
					lstyles.insert(f.styles[j]);
					if (config->lstylesMerge)
						break;
				}
			}

			if (config->lstylesMerge && lstyles.size())
				break;
		}

		for (auto it = lstyles.begin(); it != lstyles.end(); it++)
		{
			Texture lmap2(lightmap.block_width, lightmap.block_height, Texture::RGB8);

			for (int i = 0; i < lmRects.size(); i++)
			{
				const dface_t &f = faces[i];
				const Lightmap::RectI &rect = lmRects[i];
				if (rect.w * rect.h == 0)
					continue;
				std::vector<uint8_t> flmap(rect.w * rect.h * 3);
				memset(&flmap[0], 0, flmap.size());
				int lmOffset = 0;
				for (int s = 0; s < LM_STYLES && f.styles[s] != 255; s++)
				{
					if (config->lstylesMerge || f.styles[s] == *it)
					{
						for (int l = 0; l < flmap.size(); l++)
						{
							int val = flmap[l] + lightmapPixels[f.lightofs + lmOffset + l];
							flmap[l] = (val > 255) ? 255 : val;
						}
					}
					lmOffset += rect.w * rect.h * 3;
				}

				const uint8_t *data = flmap.data();
				uint8_t *dst = lmap2.get(rect.x, rect.y);
				for (int i = 0; i < rect.h; i++)
				{
					memcpy(dst, data, rect.w * 3);
					dst += lmap2.width * 3;
					data += rect.w * 3;
				}
			}
			if (config->lstylesMerge)
				lmap2.save((std::string(name) + "_merged_lightmap.png").c_str(), config->verbose);
			else
				lmap2.save((std::string(name) + "_style" + std::to_string(*it) + "_lightmap.png").c_str(), config->verbose);
		}

		if (config->lstylesMerge && lstyles.empty() && config->verbose)
			printf("No lightstyles found\n");
	}

	return true;
}

void Map::hlbsp_loadTextures(FILE *f, int fileofs, int filelen, std::vector<WadFile> wads, bool verbose)
{
	using namespace hlbsp;
	if (!filelen)
		return;

	fseek(f, fileofs, SEEK_SET);
	int32_t texCount = 0;
	fread(&texCount, sizeof(uint32_t), 1, f);
	if (verbose)
		printf("Load %d textures\n", texCount);
	textures.resize(texCount);
	std::vector<int32_t> texOffs(texCount);
	fread(&texOffs[0], sizeof(uint32_t), texCount, f);

	for (int i = 0; i < texCount; i++)
	{
		if (texOffs[i] == -1)
		{
			fprintf(stderr, "Error: texture %d bad offset\n", i);
			textures[i].name = "default";
			textures[i].width = 16;
			textures[i].height = 16;
			continue;
		}

		fseek(f, fileofs + texOffs[i], SEEK_SET);
		mip_t texHeader;
		fread(&texHeader, sizeof(texHeader), 1, f);

		textures[i].name = texHeader.name;
		textures[i].width = texHeader.width;
		textures[i].height = texHeader.height;

		std::vector<uint8_t> buffer;

		if (texHeader.offsets[0] == 0)
		{
			// reference to a texture stored inside a *.wad
			int w = 0;
			for (; w < wads.size(); w++)
			{
				if (wads[w].findLump(texHeader.name, buffer))
					break;
			}

			if (w == wads.size())
				continue; // not found
		}
		else
		{
			buffer.resize(sizeof(mip_t) + ((texHeader.width * texHeader.height * 85) >> 6) + sizeof(uint16_t) + 256 * 3);
			fseek(f, fileofs + texOffs[i], SEEK_SET);
			fread(&buffer[0], buffer.size(), 1, f);
		}

		LoadMipTexture(&buffer[0], textures[i]);

		if (verbose)
			printf("Loaded texture: %s \t%dx%d\n", textures[i].name.c_str(), textures[i].width, textures[i].height);
	}
}

void Map::parseEntities(const char *src, size_t size)
{
	wadNames.clear();

	bool isWorldSpawn = true;

	Parser parser(src, size);
	std::string token;

	while (parser.getToken(token))
	{
		if (token != "{")
		{
			fprintf(stderr, "Error: parseEntities expected '{', got \"%s\"\n", token.c_str());
			return;
		}

		std::string model;
		std::string origin;

		while (true)
		{
			if (!parser.getToken(token))
			{
				fprintf(stderr, "Error: parseEntities unexpected end of file\n");
				return;
			}

			if (token == "}")
				break;

			std::string keyname = token;

			if (!parser.getToken(token))
			{
				fprintf(stderr, "Error: parseEntities unexpected end of file\n");
				return;
			}

			if (token == "}")
			{
				fprintf(stderr, "Error: parseEntities unexpected '}' without data\n");
				return;
			}
			//printf("%s = \"%s\"\n", keyname.c_str(), token.c_str());

			if (isWorldSpawn && keyname == "wad")
			{
				size_t s = 0;
				while(true)
				{
					size_t e = token.find(';', s);
					if (e == std::string::npos)
						e = token.size();

					if (e - s > 0)
					{
						std::string temp = token.substr(s, e - s);
						size_t p = temp.find_last_of("/\\");
						if (p != std::string::npos)
							temp = temp.substr(p + 1);
						wadNames.push_back(temp);
					}
					else
						break;
					s = e + 1;
				}
			}
			else if (keyname == "model")
			{
				model = token;
			}
			else if (keyname == "origin")
			{
				origin = token;
			}
		}
		isWorldSpawn = false;

		if (model.size() && model[0] == '*' && origin.size())
		{
			vec3_t v;
			int modelId = atoi(&model[1]);
			if ((modelId > 0 && modelId < models.size()) && sscanf(origin.c_str(), "%f %f %f", &v.x, &v.y, &v.z) == 3)
			{
				models[modelId].position = v;
			}
		}
	}
}

