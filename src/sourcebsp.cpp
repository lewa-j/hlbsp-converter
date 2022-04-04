// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)

#include "sourcebsp.h"
#include "lightmap.h"
#include <map>

bool Map::load_vbsp(FILE *f, const char *name, LoadConfig *config)
{
	using namespace srcbsp;

	bspHeader_t header;
	fread(&header, sizeof(header), 1, f);
	printf("vbsp version %d\n", header.version);

	if (header.version < 19 || header.version > 21)
	{
		fprintf(stderr, "Error: unknown vbsp version %d\n", header.version);
		fclose(f);
		return false;
	}

	std::vector<char> entitiesText;
	std::vector<bspTexData_t> texdatas;
	std::vector<vec3_t> bspVertices;
	std::vector<bspTexInfo_t> texinfos;
	std::vector<bspFace_t> faces;
	std::vector<uint8_t> lightmapPixels;
	std::vector<uint16_t> edges;
	std::vector<int32_t> surfedges;
	std::vector<bspModel_t> bspModels;
	std::vector<vec3_t> normals;
	std::vector<uint16_t> normalInds;
	std::vector<char> texDataStings;
	std::vector<int> texDataStingTable;

#define READ_LUMP(to, id) \
	to.resize(header.lumps[id].filelen / sizeof(to[0])); \
	if(header.lumps[id].filelen) \
	{ \
		fseek(f, header.lumps[id].fileofs, SEEK_SET); \
		fread(&to[0], header.lumps[id].filelen, 1, f); \
	}

	READ_LUMP(entitiesText, LUMP_ENTITIES);
	READ_LUMP(texdatas, LUMP_TEXDATA);
	READ_LUMP(bspVertices, LUMP_VERTEXES);
	READ_LUMP(texinfos, LUMP_TEXINFO);
	if (header.lumps[LUMP_LIGHTING].filelen || !header.lumps[LUMP_LIGHTING_HDR].filelen)
	{
		READ_LUMP(faces, LUMP_FACES);
		READ_LUMP(lightmapPixels, LUMP_LIGHTING);
	}
	else
	{
		READ_LUMP(faces, LUMP_FACES_HDR);
		READ_LUMP(lightmapPixels, LUMP_LIGHTING_HDR);
	}
	READ_LUMP(edges, LUMP_EDGES);
	READ_LUMP(surfedges, LUMP_SURFEDGES);
	READ_LUMP(bspModels, LUMP_MODELS);
	READ_LUMP(normals, LUMP_VERTNORMALS);
	READ_LUMP(normalInds, LUMP_VERTNORMALINDICES);
	READ_LUMP(texDataStings, LUMP_TEXDATA_STRING_DATA);
	READ_LUMP(texDataStingTable, LUMP_TEXDATA_STRING_TABLE);
#undef READ_LUMP

	fclose(f);

	models.resize(bspModels.size());
	parseEntities(&entitiesText[0], entitiesText.size());

	materials.resize(texdatas.size());
	for (int i = 0; i < texdatas.size(); i++)
	{
		materials[i].name = texDataStings.data() + texDataStingTable[texdatas[i].nameStringTableID];
		for (auto &c : materials[i].name)
			c = std::tolower(c);
		materials[i].lightmapped = (lightmapPixels.size() != 0);
	}

	std::vector<Lightmap::RectI> lmRects(faces.size());
	std::vector<int> normalOffsets(faces.size());

	Lightmap lightmap(config->lightmapSize, false, true);

	int normalOffset = 0;
	for (int mi = 0; mi < bspModels.size(); mi++)
	{
		const bspModel_t &modIn = bspModels[mi];

		for (int fi = modIn.firstFace; fi < modIn.firstFace + modIn.faceCount; fi++)
		{
			const bspFace_t &f = faces[fi];
			const bspTexInfo_t &ti = texinfos[f.texInfo];
			normalOffsets[fi] = normalOffset;
			normalOffset += f.edgesCount;

			lmRects[fi] = { 0,0,0,0 };
			if (config->skipSky && (ti.flags & SURF_SKY))
				continue;

			if (f.lightOfs != -1)
			{
				lmRects[fi].w = f.lightmapSize[0] + 1;
				lmRects[fi].h = f.lightmapSize[1] + 1;
			}
		}
	}

	if (lightmapPixels.size())
	{
		lightmap.pack(lmRects, config->lightmapSize);
		lightmap.initBlock();
	}

	int indOffset = 0;
	for (int mi = 0; mi < bspModels.size(); mi++)
	{
		const bspModel_t &modIn = bspModels[mi];

		std::map<int, std::vector<int> > materialFaces;

		for (int fi = modIn.firstFace; fi < modIn.firstFace + modIn.faceCount; fi++)
		{
			const bspFace_t &f = faces[fi];
			const bspTexInfo_t &ti = texinfos[f.texInfo];
			if (config->skipSky && (ti.flags & SURF_SKY))
				continue;
			materialFaces[ti.texData].push_back(fi);
		}

		models[mi].meshes.push_back({});
		mesh_t &mesh = models[mi].meshes[0];
		mesh.vertOffset = vertices.size();
		mesh.offset = indOffset;
		int curV = 0;
		for (auto &mat : materialFaces)
		{
			submesh_t submesh{ 0 };
			submesh.material = mat.first;
			submesh.offset = indOffset;

			for (int i = 0; i < mat.second.size(); i++)
			{
				const bspFace_t &f = faces[mat.second[i]];
				const bspTexInfo_t &ti = texinfos[f.texInfo];
				const bspTexData_t &td = texdatas[ti.texData];
				Lightmap::RectI &rect = lmRects[mat.second[i]];

				if (f.lightOfs != -1 && lightmapPixels.size())
				{
					lightmap.write(rect, &lightmapPixels[f.lightOfs]);
				}

				for (int ei = 0; ei < f.edgesCount; ei++)
				{
					int se = surfedges[f.firstEdge + ei];
					int v = edges[abs(se) * 2 + (se > 0 ? 0 : 1)];

					vert_t vert{};
					vert.pos = bspVertices[v];
					vert.norm = normals[normalInds[normalOffsets[mat.second[i]] + ei]];
					vert.uv[0] = ((vert.pos.x * ti.textureVecS.x + vert.pos.y * ti.textureVecS.y + vert.pos.z * ti.textureVecS.z) + ti.textureOffS) / td.width;
					vert.uv[1] = ((vert.pos.x * ti.textureVecT.x + vert.pos.y * ti.textureVecT.y + vert.pos.z * ti.textureVecT.z) + ti.textureOffT) / td.height;
					if (f.lightOfs != -1) {
						vert.uv2[0] = ((vert.pos.x * ti.lightmapVecS.x + vert.pos.y * ti.lightmapVecS.y + vert.pos.z * ti.lightmapVecS.z) + ti.lightmapOffS + 0.5f - f.lightmapMins[0] + rect.x) / lightmap.block_width;
						vert.uv2[1] = ((vert.pos.x * ti.lightmapVecT.x + vert.pos.y * ti.lightmapVecT.y + vert.pos.z * ti.lightmapVecT.z) + ti.lightmapOffT + 0.5f - f.lightmapMins[1] + rect.y) / lightmap.block_height;
					}
					vertices.push_back(vert);
				}

				for (int ei = 0; ei < f.edgesCount - 2; ei++)
				{
					indices32.push_back(curV);
					indices32.push_back(curV + ei + 2);
					indices32.push_back(curV + ei + 1);
				}
				curV += f.edgesCount;
				submesh.count += (f.edgesCount - 2) * 3;
				indOffset += (f.edgesCount - 2) * 3;
			}
			mesh.count += submesh.count;
			mesh.submeshes.push_back(submesh);
		}
		mesh.vertCount = curV;
	}

	if(lightmapPixels.size())
		lightmap.uploadBlock(name);

	return true;
}
