// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)

#include "sourcebsp.h"
#include <map>
#include "map.h"
#include "lightmap.h"
#include <cfloat>
#include <functional>
#include <format>
#include <cstring>

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

	if (header.lumps[LUMP_LEAFS].version != 0 && header.lumps[LUMP_LEAFS].version != 1)
	{
		fprintf(stderr, "Error: unknown leafs lemp version %d\n", header.lumps[LUMP_LEAFS].version);
		fclose(f);
		return false;
	}

	std::vector<char> entitiesText;
	std::vector<bspTexData_t> texdatas;
	std::vector<vec3_t> bspVertices;
	std::vector<bspNode_t> bspNodes;
	std::vector<bspTexInfo_t> texinfos;
	std::vector<bspFace_t> faces;
	std::vector<uint8_t> lightmapPixels;
	std::vector<bspLeaf_v1_t> bspLeafs;
	std::vector<uint16_t> edges;
	std::vector<int32_t> surfedges;
	std::vector<bspModel_t> bspModels;
	std::vector<uint16_t> bspLeafFaces;
	std::vector<bspDispInfo_t> dispInfos;
	std::vector<vec3_t> normals;
	std::vector<uint16_t> normalInds;
	std::vector<bspDispVert_t> bspDispVerts;
	std::vector<char> texDataStings;
	std::vector<int> texDataStingTable;

	std::vector<bspArea_t> bspAreas;
	std::vector<bspAreaPortal_t> bspAreaPortals;
	std::vector<vec3_t> bspAreaPortalVerts;

#define READ_LUMP(to, id) \
	to.resize(header.lumps[id].size / sizeof(to[0])); \
	if(header.lumps[id].size) \
	{ \
		fseek(f, header.lumps[id].offset, SEEK_SET); \
		fread(&to[0], header.lumps[id].size, 1, f); \
	}

	READ_LUMP(entitiesText, LUMP_ENTITIES);
	READ_LUMP(texdatas, LUMP_TEXDATA);
	READ_LUMP(bspVertices, LUMP_VERTEXES);
	READ_LUMP(bspNodes, LUMP_NODES);
	READ_LUMP(texinfos, LUMP_TEXINFO);
	if (header.lumps[LUMP_LIGHTING].size || !header.lumps[LUMP_LIGHTING_HDR].size)
	{
		READ_LUMP(faces, LUMP_FACES);
		READ_LUMP(lightmapPixels, LUMP_LIGHTING);
	}
	else
	{
		READ_LUMP(faces, LUMP_FACES_HDR);
		READ_LUMP(lightmapPixels, LUMP_LIGHTING_HDR);
	}

	if(header.lumps[LUMP_LEAFS].version == 0)
	{
		std::vector<bspLeaf_v0_t> tempLeafs;
		READ_LUMP(tempLeafs, LUMP_LEAFS);

		bspLeafs.resize(tempLeafs.size());
		for (int i = 0; i < tempLeafs.size(); i++)
			memcpy(&bspLeafs[i], &tempLeafs[i], sizeof(bspLeaf_v1_t));
	}
	else if (header.lumps[LUMP_LEAFS].version == 1)
	{
		READ_LUMP(bspLeafs, LUMP_LEAFS);
	}

	READ_LUMP(edges, LUMP_EDGES);
	READ_LUMP(surfedges, LUMP_SURFEDGES);
	READ_LUMP(bspModels, LUMP_MODELS);
	READ_LUMP(bspLeafFaces, LUMP_LEAFFACES);
	READ_LUMP(bspAreas, LUMP_AREAS);
	READ_LUMP(bspAreaPortals, LUMP_AREAPORTALS);
	READ_LUMP(dispInfos, LUMP_DISPINFO);
	READ_LUMP(normals, LUMP_VERTNORMALS);
	READ_LUMP(normalInds, LUMP_VERTNORMALINDICES);
	READ_LUMP(bspDispVerts, LUMP_DISP_VERTS);
	READ_LUMP(bspAreaPortalVerts, LUMP_CLIPPORTALVERTS);
	READ_LUMP(texDataStings, LUMP_TEXDATA_STRING_DATA);
	READ_LUMP(texDataStingTable, LUMP_TEXDATA_STRING_TABLE);
#undef READ_LUMP

	fclose(f);

	if (config->scan)
	{
		printf("verts: %zd\n", bspVertices.size());
		printf("texdatas: %zd\n", texdatas.size());
		printf("texinfos: %zd\n", texinfos.size());
		printf("nodes: %zd\n", bspNodes.size());
		printf("leafs: %zd v%d\n", bspLeafs.size(), header.lumps[LUMP_LEAFS].version);
		printf("faces: %zd\n", faces.size());
		printf("models: %zd\n", bspModels.size());
		printf("lightmap: %zd bytes\n", lightmapPixels.size());

		printf("areas: %zd\n", bspAreas.size());
		printf("area portals: %zd\n", bspAreaPortals.size());
		printf("portal verts: %zd\n", bspAreaPortalVerts.size());

		printf("disp infos: %zd\n", dispInfos.size());
		printf("disp verts: %zd\n", bspDispVerts.size());

		{
			int nodeFaces = 0;
			for (int i = 0; i < bspNodes.size(); i++)
			{
				bspNode_t &node = bspNodes[i];
				nodeFaces += node.facesCount;
				if (node.area != 0)
				{
					printf("node %d: area %d, faces %d\n", i, node.area, node.facesCount);
				}
			}
			printf("total faces in nodes: %d\n", nodeFaces);
		}

		{
			std::map<int, std::pair<int, int>> leafAreas;
			std::vector<int> faceToLeaf(faces.size(), -1);
			int leafFaces = 0;
			for (int i = 0; i < bspLeafs.size(); i++)
			{
				auto &leaf = bspLeafs[i];
				leafAreas[leaf.area].first++;
				leafAreas[leaf.area].second += leaf.facesCount;
				leafFaces += leaf.facesCount;

				if (leaf.contents == 1 && leaf.facesCount != 0)
					printf("weird leaf %d: contents %d, cluster %d, area %d, facesCount %d\n", i, leaf.contents, leaf.cluster, leaf.area, leaf.facesCount);
			}
			printf("total faces in leafs: %d\n", leafFaces);
			for (auto t : leafAreas)
				printf("area %d: %d leafs %d faces\n", t.first, t.second.first, t.second.second);
		}
	}

	std::vector<int> faceAreas(faces.size(), -1);

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

		if (config->scan)
		{
			printf("model %d: faces (%d-%d), node %d\n", mi, modIn.firstFace, modIn.faceCount, modIn.headNode);
		}

		std::function<void(int)> walkTree;
		int depth = 0;
		walkTree = [&](int id)
		{
			if (id < 0)
			{
				id = -id - 1;
				if (id >= bspLeafs.size())
					return;
				auto &leaf = bspLeafs[id];
				if (config->verbose && (leaf.contents != 1 || leaf.facesCount != 0))
				{
					for (int d = 0; d < depth; d++)
						putchar(' ');
					printf("leaf %d: contents %d, cluster %d, area %d, facesCount %d\n", id, leaf.contents, leaf.cluster, leaf.area, leaf.facesCount);
				}
				for (int lfi = 0; lfi < leaf.facesCount; lfi++)
				{
					int &faceArea = faceAreas[bspLeafFaces[leaf.leafFaceOffset + lfi]];
					if (faceArea == -1)
					{
						faceArea = leaf.area;
					}
					else if (faceArea != -2 && faceArea != leaf.area)
					{
						if (config->verbose)
							printf("face %d double area %d & %d\n", bspLeafFaces[leaf.leafFaceOffset + lfi], faceArea, leaf.area);
						faceArea = -2;
						continue;
					}
				}
				return;
			}
			if (id >= bspNodes.size())
				return;
			bspNode_t &node = bspNodes[id];
			if (config->verbose)
			{
				for (int d = 0; d < depth; d++)
					putchar(' ');
				printf("node %d: area %d, faces %d, c %d %d\n", id, node.area, node.facesCount, node.children[0], node.children[1]);
			}
			depth++;
			walkTree(node.children[0]);
			walkTree(node.children[1]);
			depth--;
		};

		walkTree(modIn.headNode);

		for (uint32_t fi = modIn.firstFace; fi < modIn.firstFace + modIn.faceCount; fi++)
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

	if (config->scan)
		return false;

	if (lightmapPixels.size())
	{
		lightmap.pack(lmRects, config->lightmapSize);
		lightmap.initBlock();
	}

	int indOffset = 0;
	for (int mi = 0; mi < bspModels.size(); mi++)
	{
		const bspModel_t &modIn = bspModels[mi];

		std::map<int, std::map<int, std::vector<int> > > areaMaterialFaces;

		for (uint32_t fi = modIn.firstFace; fi < modIn.firstFace + modIn.faceCount; fi++)
		{
			const bspFace_t &f = faces[fi];
			const bspTexInfo_t &ti = texinfos[f.texInfo];
			if (config->skipSky && (ti.flags & SURF_SKY))
				continue;
			int area = faceAreas[fi];
			if (area == -1)
				area = 0;
			areaMaterialFaces[area][ti.texData].push_back(fi);
		}

		for (auto &area : areaMaterialFaces)
		{
			bool hasDisp = false;

			models[mi].meshes.push_back({});
			mesh_t &mesh = models[mi].meshes.back();
			mesh.name = (area.first == -2) ? "area_mixed" : std::format("area_{}", area.first);
			mesh.vertOffset = (int)vertices.size();
			mesh.offset = indOffset;
			int curV = 0;
			for (auto &mat : area.second)
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
			mesh_t &dmesh = models[mi].dispMeshes.back();
			dmesh.name = (area.first == -2) ? "disp_area_mixed" : std::format("disp_area_{}", area.first);
			dmesh.vertOffset = (int)dispVertices.size();
			dmesh.offset = indOffset;
			curV = 0;

			for (auto &mat : area.second)
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

					int width = (1 << di.power) + 1;
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
							vec3_t norm{ 0.0f,0.0f,0.0f };
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
							if ((cv - curV) % 2) {
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
	}

	if(lightmapPixels.size())
		lightmap.uploadBlock(name, config->verbose);

	return true;
}
