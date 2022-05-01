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
	std::vector<bspDispInfo_t> dispInfos;
	std::vector<vec3_t> normals;
	std::vector<uint16_t> normalInds;
	std::vector<bspDispVert_t> bspDispVerts;
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
	READ_LUMP(dispInfos, LUMP_DISPINFO);
	READ_LUMP(normals, LUMP_VERTNORMALS);
	READ_LUMP(normalInds, LUMP_VERTNORMALINDICES);
	READ_LUMP(bspDispVerts, LUMP_DISP_VERTS);
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
		bool hasDisp = false;

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

				if (f.dispInfo != -1)
				{
					hasDisp = true;
					continue;
				}

				for (int ei = 0; ei < f.edgesCount; ei++)
				{
					int se = surfedges[f.firstEdge + ei];
					int v = edges[abs(se) * 2 + (se > 0 ? 0 : 1)];

					vert_t vert{};
					vert.pos = bspVertices[v];
					vert.norm = normals[normalInds[normalOffsets[mat.second[i]] + ei]];
					vert.uv = { (vert.pos.dot(ti.textureVecS) + ti.textureOffS) / td.width, (vert.pos.dot(ti.textureVecT) + ti.textureOffT) / td.height };
					if (f.lightOfs != -1) {
						vert.uv2.x = (vert.pos.dot(ti.lightmapVecS) + ti.lightmapOffS + 0.5f - f.lightmapMins[0] + rect.x) / lightmap.block_width;
						vert.uv2.y = (vert.pos.dot(ti.lightmapVecT) + ti.lightmapOffT + 0.5f - f.lightmapMins[1] + rect.y) / lightmap.block_height;
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
			if (submesh.count)
			{
				mesh.count += submesh.count;
				mesh.submeshes.push_back(submesh);
			}
		}
		mesh.vertCount = curV;

		if (!hasDisp)
			continue;

		models[mi].dispMeshes.push_back({});
		mesh_t &dmesh = models[mi].dispMeshes[0];
		dmesh.vertOffset = dispVertices.size();
		dmesh.offset = indOffset;
		curV = 0;
		for (auto &mat : materialFaces)
		{
			submesh_t submesh{ 0 };
			submesh.material = mat.first;
			submesh.offset = indOffset;

			for (int i = 0; i < mat.second.size(); i++)
			{
				const bspFace_t &f = faces[mat.second[i]];

				if (f.dispInfo == -1)
					continue;
				const bspTexInfo_t &ti = texinfos[f.texInfo];
				const bspTexData_t &td = texdatas[ti.texData];
				const Lightmap::RectI &rect = lmRects[mat.second[i]];
				const bspDispInfo_t &di = dispInfos[f.dispInfo];

				if (f.edgesCount > 4)
				{
					fprintf(stderr, "Error: unexpected edge count (%d) on face %d with displacement %d\n", f.edgesCount, mat.second[i], f.dispInfo);
					continue;
				}
				vert_t tempVerts[4];
				int vertIdShift = 0;
				float nearestDist = FLT_MAX;
				for (int ei = 0; ei < f.edgesCount; ei++)
				{
					int se = surfedges[f.firstEdge + ei];
					int v = edges[abs(se) * 2 + (se > 0 ? 0 : 1)];

					vert_t &vert = tempVerts[ei];
					vert.pos = bspVertices[v];
					vert.norm = normals[normalInds[normalOffsets[mat.second[i]] + ei]];
					vert.uv = { (vert.pos.dot(ti.textureVecS) + ti.textureOffS) / td.width, (vert.pos.dot(ti.textureVecT) + ti.textureOffT) / td.height };

					float dist = di.startPos.dist2(vert.pos);
					if (dist < nearestDist) {
						nearestDist = dist;
						vertIdShift = ei;
					}
				}

				int i0 = vertIdShift;
				int i1 = (vertIdShift + 1) % 4;
				int i2 = (vertIdShift + 2) % 4;
				int i3 = (vertIdShift + 3) % 4;
				if (f.lightOfs != -1)
				{
					tempVerts[i0].uv2 = { (rect.x + 0.5f) / lightmap.block_width, (rect.y + 0.5f) / lightmap.block_height };
					tempVerts[i1].uv2 = { (rect.x + 0.5f) / lightmap.block_width, (rect.y + rect.h - 0.5f) / lightmap.block_height };
					tempVerts[i2].uv2 = { (rect.x + rect.w - 0.5f) / lightmap.block_width, (rect.y + rect.h - 0.5f) / lightmap.block_height };
					tempVerts[i3].uv2 = { (rect.x + rect.w - 0.5f) / lightmap.block_width, (rect.y + 0.5f) / lightmap.block_height };
				}

				int width = (1 << di.power)+1;
				for (int y = 0; y < width; y++)
				{
					float yf = y / (float)(width - 1);
					vec3_t posy1 = vec3_t::lerp(tempVerts[i0].pos, tempVerts[i1].pos, yf);
					vec3_t posy2 = vec3_t::lerp(tempVerts[i3].pos, tempVerts[i2].pos, yf);
					vec2_t uvy1 = vec2_t::lerp(tempVerts[i0].uv, tempVerts[i1].uv, yf);
					vec2_t uvy2 = vec2_t::lerp(tempVerts[i3].uv, tempVerts[i2].uv, yf);
					vec2_t lmuvy1 = vec2_t::lerp(tempVerts[i0].uv2, tempVerts[i1].uv2, yf);
					vec2_t lmuvy2 = vec2_t::lerp(tempVerts[i3].uv2, tempVerts[i2].uv2, yf);

					int vo = di.dispVertOffset + y * width;
					for (int x = 0; x < width; x++)
					{
						float xf = x / (float)(width - 1);
						bspDispVert_t &dv = bspDispVerts[vo + x];

						dispVert_t vert;
						vert.pos = vec3_t::lerp(posy1, posy2, xf);
						vert.pos = { vert.pos.x + dv.vector.x * dv.dist, vert.pos.y + dv.vector.y * dv.dist, vert.pos.z + dv.vector.z * dv.dist };
						vert.uv = vec2_t::lerp(uvy1, uvy2, xf);
						vert.uv2 = vec2_t::lerp(lmuvy1, lmuvy2, xf);
						vert.color = { 1.0f, 1.0f, 1.0f };
						vert.alpha = dv.alpha / 255.0f;
						dispVertices.push_back(vert);
					}
				}

				// normals
				int cv = curV;
				for (int y = 0; y < width; y++)
				{
					for (int x = 0; x < width; x++)
					{
						auto rpos = [&](int dx, int dy)
						{
							return dispVertices[cv + (y + dy) * width + x + dx].pos;
						};

						dispVert_t &vert = dispVertices[cv + y * width + x];
						vec3_t norm{0.0f,0.0f,0.0f};
						int n = 0;
						if (y + 1 < width && x + 1 < width)
						{
							vec3_t v0 = rpos(0, 1) - vert.pos;
							vec3_t v1 = rpos(1, 0) - vert.pos;
							norm = norm + v1.cross(v0).normalize();
							n++;

							v0 = rpos(0, 1) - rpos(1, 0);
							v1 = rpos(1, 1) - rpos(1, 0);
							norm = norm + v1.cross(v0).normalize();
							n++;
						}

						if (x > 0 && y + 1 < width)
						{
							vec3_t v0 = rpos(-1, 1) - rpos(-1, 0);
							vec3_t v1 = vert.pos - rpos(-1, 0);
							norm = norm + v1.cross(v0).normalize();
							n++;

							v0 = rpos(-1, 1) - vert.pos;
							v1 = rpos(0, 1) - vert.pos;
							norm = norm + v1.cross(v0).normalize();
							n++;
						}

						if (x > 0 && y > 0)
						{
							vec3_t v0 = rpos(-1, 0) - rpos(-1, -1);
							vec3_t v1 = rpos(0, -1) - rpos(-1, -1);
							norm = norm + v1.cross(v0).normalize();
							n++;

							v0 = rpos(-1, 0) - rpos(0, -1);
							v1 = vert.pos - rpos(0, -1);
							norm = norm + v1.cross(v0).normalize();
							n++;
						}

						if (x + 1 < width && y > 0)
						{
							vec3_t v0 = vert.pos - rpos(0, -1);
							vec3_t v1 = rpos(1, -1) - rpos(0, -1);
							norm = norm + v1.cross(v0).normalize();
							n++;

							v0 = vert.pos - rpos(1, -1);
							v1 = rpos(1, 0) - rpos(1, -1);
							norm = norm + v1.cross(v0).normalize();
							n++;
						}

						norm *= (1.0f / n);
						vert.norm = norm;
					}
				}

				cv = curV;
				for (int y = 0; y < width - 1; y++) {
					for (int x = 0; x < width - 1; x++) {
						if (cv % 2) {
							indices32.push_back(cv);
							indices32.push_back(cv + 1);
							indices32.push_back(cv + width);
							indices32.push_back(cv + 1);
							indices32.push_back(cv + width + 1);
							indices32.push_back(cv + width);
						}
						else {
							indices32.push_back(cv);
							indices32.push_back(cv + width + 1);
							indices32.push_back(cv + width);
							indices32.push_back(cv);
							indices32.push_back(cv + 1);
							indices32.push_back(cv + width + 1);
						}
						cv++;
					}
					cv++;
				}

				curV += width * width;
				submesh.count += (width - 1) * (width - 1) * 6;
				indOffset += (width - 1) * (width - 1) * 6;
			}
			if (submesh.count)
			{
				dmesh.count += submesh.count;
				dmesh.submeshes.push_back(submesh);
			}
		}
		dmesh.vertCount = curV;
	}

	if(lightmapPixels.size())
		lightmap.uploadBlock(name);

	return true;
}
