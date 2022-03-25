// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "hlbsp.h"
#include "lightmap.h"
#include <stdio.h>
#include <map>

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

	printf("Load %d models\n", (int)bspModels.size());

	hlbsp_loadTextures(f, header.lumps[LUMP_TEXTURES].fileofs, header.lumps[LUMP_TEXTURES].filelen);

	fclose(f);

	materials.resize(textures.size());
	for (int i = 0; i < materials.size(); i++)
	{
		materials[i].name = textures[i].name;
		materials[i].texture = i;
	}

	//save lightmap packing info and reuse it for lightsyles export
	struct surface_t
	{
		int offs[2];
		int size[2];
	};
	std::vector<surface_t> surfaces(faces.size());

	Lightmap lightmap(config->lightmapSize, lightmapVecs.size() != 0);

	lightmap.initBlock();

	int numedges = edges.size();
	int indicesOffset = 0;
	models.resize(bspModels.size());
	for (int mi = 0; mi < bspModels.size(); mi++)
	{
		const auto &m = bspModels[mi];
		// sort faces by texture and batch into a single mesh
		std::map<int, std::vector<int> > materialFaces;

		for (int fi = m.firstface; fi < m.firstface + m.numfaces; fi++)
		{
			const auto &f = faces[fi];
			const auto &ti = texinfos[f.texinfo];
			if (config->skipSky && textures[ti.miptex].name == "sky")
				continue;
			materialFaces[ti.miptex].push_back(fi);
		}

		models[mi].push_back({});
		model_t *model = &models[mi].back();

		model->count = 0;
		model->offset = indicesOffset;
		model->vertOffset = vertices.size();
		int indVertOffset = 0;
		for (auto &mat : materialFaces)
		{
			submesh_t submesh;
			submesh.material = mat.first;
			submesh.offset = indicesOffset;
			submesh.count = 0;

			for (int i = 0; i < mat.second.size(); i++)
			{
				const auto &f = faces[mat.second[i]];
				const auto &ti = texinfos[f.texinfo];

				if (config->uint16Inds && (indVertOffset + f.numedges >= UINT16_MAX - 1))
				{
					model->vertCount = vertices.size() - model->vertOffset;
					if (submesh.count)
					{
						indicesOffset += submesh.count;
						model->count += submesh.count;
						model->submeshes.push_back(submesh);
					}

					models[mi].push_back({});
					model = &models[mi].back();

					model->count = 0;
					model->offset = indicesOffset;
					model->vertOffset = vertices.size();
					indVertOffset = 0;

					submesh.material = mat.first;
					submesh.offset = indicesOffset;
					submesh.count = 0;
				}

				int faceVertOffset = vertices.size();

				float min_uv[2]{ FLT_MAX, FLT_MAX };
				float max_uv[2]{ -FLT_MAX, -FLT_MAX };
				for (int j = 0; j < f.numedges; j++)
				{
					int e = surfedges[f.firstedge + j];
					if (e >= numedges || e <= -numedges)
					{
						fprintf(stderr, "Error: model %d face %d bad edge %d\n", mi, mat.second[i], e);
						return -1;
					}
					int vi = edges[abs(e)].v[(e > 0 ? 0 : 1)];
					if (vi >= bspVertices.size()) {
						fprintf(stderr, "Error: model %d face %d bad vertex index %d\n", mi, mat.second[i], vi);
						return -1;
					}
					vert_t v{ bspVertices[vi] };
					for (int k = 0; k < 2; k++)
					{
						v.uv[k] = (v.pos.x * ti.vecs[k][0] + v.pos.y * ti.vecs[k][1] + v.pos.z * ti.vecs[k][2]) + ti.vecs[k][3];
						min_uv[k] = fmin(min_uv[k], v.uv[k]);
						max_uv[k] = fmax(max_uv[k], v.uv[k]);
					}
					vertices.push_back(v);
				}

				auto &fl = surfaces[mat.second[i]];
				fl = { 0,0,0,0 };

				if (f.lightofs != -1 && f.styles[0] != 255)
				{
					int sampleSize = lmSampleSize;
					if (ti.faceInfo >= 0 && ti.faceInfo < faceInfos.size())
					{
						sampleSize = faceInfos[ti.faceInfo].textureStep;
					}

					int bmins[2]{ floor(min_uv[0] / sampleSize), floor(min_uv[1] / sampleSize) };
					int bmaxs[2]{ ceil(max_uv[0] / sampleSize), ceil(max_uv[1] / sampleSize) };
					fl.size[0] = bmaxs[0] - bmins[0] + 1;
					fl.size[1] = bmaxs[1] - bmins[1] + 1;

					if (!lightmap.allocBlock(fl.size[0], fl.size[1], fl.offs[0], fl.offs[1]))
					{
						printf("Warning: lightmap altas is full, creating additional atlas. Use '-lm' parameter if you want to fit all lighting into one atlas.\n");
						lightmap.uploadBlock(name);
						lightmap.initBlock();
						lightmap.allocBlock(fl.size[0], fl.size[1], fl.offs[0], fl.offs[1]);
					}

					lightmap.write(fl.size[0], fl.size[1], fl.offs[0], fl.offs[1], &lightmapPixels[f.lightofs], lightmapVecs.size() ? &lightmapVecs[f.lightofs] : nullptr);
					for (int j = 0; j < f.numedges; j++)
					{
						vert_t &v = vertices[faceVertOffset + j];
						for (int k = 0; k < 2; k++)
						{
							v.uv2[k] = (v.uv[k] - bmins[k] * sampleSize + fl.offs[k] * sampleSize + sampleSize * 0.5f) / (lightmap.block_size * sampleSize);
						}
					}
				}

				const Texture &tex = textures[ti.miptex];
				if (tex.width && tex.height)
				{
					for (int j = 0; j < f.numedges; j++)
					{
						vert_t &v = vertices[faceVertOffset + j];
						v.uv[0] /= tex.width;
						v.uv[1] /= tex.height;
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
				model->submeshes.push_back(submesh);

			model->count += submesh.count;
			indicesOffset += submesh.count;
		}
		model->vertCount = vertices.size() - model->vertOffset;
	}
	lightmap.uploadBlock(name);

	int activeStyles = 0;
	for (int i = 0; i < faces.size(); i++)
	{
		const dface_t &f = faces[i];
		for (int j = 0; j < LM_STYLES; j++)
		{
			if (f.styles[j] != 0 && f.styles[j] != 255 && (config->lstylesAll || f.styles[j] == config->lstyle))
			{
				activeStyles++;
				break;
			}
			if (activeStyles)
				break;
		}
	}

	if(activeStyles)
	{
		Texture lmap2(lightmap.block_size, lightmap.block_size, Texture::RGB8);

		for (int i = 0; i < surfaces.size(); i++)
		{
			const dface_t &f = faces[i];
			const surface_t &fl = surfaces[i];
			if (fl.size[0] * fl.size[1] == 0)
				continue;
			std::vector<uint8_t> flmap(fl.size[0] * fl.size[1] * 3);
			memset(&flmap[0], 0, flmap.size());
			int lmOffset = 0;
			for (int s = 0; s < LM_STYLES && f.styles[s] != 255; s++)
			{
				if (config->lstylesAll || f.styles[s] == config->lstyle)
				{
					for (int l = 0; l < flmap.size(); l++)
					{
						int val = flmap[l] + lightmapPixels[f.lightofs + lmOffset + l];
						flmap[l] = (val > 255) ? 255 : val;
					}
				}
				lmOffset += fl.size[0] * fl.size[1] * 3;
			}

			const uint8_t *data = flmap.data();
			uint8_t *dst = lmap2.get(fl.offs[0], fl.offs[1]);
			for (int i = 0; i < fl.size[1]; i++)
			{
				memcpy(dst, data, fl.size[0] * 3);
				dst += lmap2.width * 3;
				data += fl.size[0] * 3;
			}
		}
		if (config->lstylesAll)
			lmap2.save((std::string(name) + "_styles_lightmap.png").c_str());
		else
			lmap2.save((std::string(name) + "_style" + std::to_string(config->lstyle) + "_lightmap.png").c_str());
	}

	return true;
}

void Map::hlbsp_loadTextures(FILE *f, int fileofs, int filelen)
{
	using namespace hlbsp;
	if (!filelen)
		return;

	fseek(f, fileofs, SEEK_SET);
	int32_t texCount = 0;
	fread(&texCount, sizeof(uint32_t), 1, f);
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

		if (texHeader.offsets[0] == 0)
		{
			// reference to a texture stored inside a *.wad
			continue;
		}

		bool hasAlpha = texHeader.name[0] == '{';
		textures[i].create(texHeader.width, texHeader.height, hasAlpha ? Texture::RGBA8 : Texture::RGB8);

		int len = texHeader.width * texHeader.height;
		std::vector<uint8_t> texData(((len * 85) >> 6) + sizeof(uint16_t) + 256 * 3);
		fseek(f, fileofs + texOffs[i] + texHeader.offsets[0], SEEK_SET);
		fread(&texData[0], texData.size(), 1, f);
		const uint8_t *ids = &texData[0];
		// two bytes before palette is a count of colors but it is always 256
		const uint8_t *pal = &texData[((len * 85) >> 6) + sizeof(uint16_t)];
		uint8_t *dst = &textures[i].data[0];

		// decode 8bit paletted texture into 24bit rgb or 32bit rgba
		for (int j = 0; j < len; j++)
		{
			int col = ids[j] * 3;
			dst[0] = pal[col + 0];
			dst[1] = pal[col + 1];
			dst[2] = pal[col + 2];
			if (hasAlpha)
			{
				if (col == (255 * 3))
					dst[0] = dst[1] = dst[2] = dst[3] = 0;
				else
					dst[3] = 255;
				dst += 4;
			}
			else
				dst += 3;
		}

		printf("Loaded texture: %s \t%dx%d\n", texHeader.name, texHeader.width, texHeader.height);
	}
}

