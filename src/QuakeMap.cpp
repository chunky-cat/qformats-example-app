#include "QuakeMap.h"
#include <raymath.h>
#include <raylib.h>

#include <iostream>
#include <locale>

#define LIGHTMAPPER_IMPLEMENTATION
#define LM_DEBUG_INTERPOLATION
#include <OpenGL/gl3.h>

string to_lower(string s)
{
	for (char& c : s)
		c = tolower(c);
	return s;
}

qformats::textures::ITexture* QuakeMap::onTextureRequest(std::string name)
{
	Texture2D rayTex;
	Image image;
	bool foundInWAD = false;
	std::locale loc;
	if (mapInstance.HasWads())
	{
		auto qt = wadMgr.FindTexture(to_lower(name));
		if (qt != nullptr)
		{
			size_t size = qt->raw.size() * sizeof(qformats::wad::color);
			image.data = RL_MALLOC(size);
			// Allocate required memory in bytes
			memcpy(image.data, &qt->raw[0], size);
			image.width = qt->width;
			image.height = qt->height;
			image.mipmaps = 1;
			image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
			rayTex = LoadTextureFromImage(image);
			foundInWAD = true;
			MemFree(image.data);
		}
	}

	if (!foundInWAD)
	{
		rayTex = LoadTexture((opts.texturePath + name + ".png").c_str());
		if (rayTex.id == -1)
			return nullptr;
	}
	GenTextureMipmaps(&rayTex);
	SetTextureFilter(rayTex, TEXTURE_FILTER_ANISOTROPIC_16X);
	auto tex = new qformats::textures::Texture<Material>();

	Material mat = LoadMaterialDefault();
	mat.maps->texture = rayTex;
	mat.maps->color = WHITE;
	tex->SetData(mat);
	tex->SetWidth(rayTex.width);
	tex->SetHeight(rayTex.height);
	return tex;
}

void QuakeMap::DrawQuakeSolids()
{
	Vector3 position = { 0.0f, 0.0f, 0.0f };
	for (const auto& m : models)
	{
		if (opts.wireframe)
		{
			DrawModelWires(m.model, position, 1, WHITE);
		}
		else
		{
			DrawModel(m.model, position, 1, WHITE);
		}

		if (opts.showVerts)
		{
			for (int i = 0; i < m.model.meshCount; i++)
			{
				auto mesh = m.model.meshes[i];
				for (int j = 0; j < mesh.triangleCount; j += 3)
				{
					Vector3 v{ mesh.vertices[j], mesh.vertices[j + 1], mesh.vertices[j + 2] };
					DrawSphere(v, 0.02, BLUE);
				}
			}
		}
	}
}

void QuakeMap::LoadMapFromFile(std::string fileName)
{
	mapInstance = qformats::map::QMap();
	mapInstance.LoadFile(fileName);

	mapInstance.ExcludeTextureSurface("CLIP");
	mapInstance.ExcludeTextureSurface("TRIGGER");
	mapInstance.ExcludeTextureSurface("clip");
	mapInstance.ExcludeTextureSurface("trigger");

	mapInstance.GenerateGeometry(true);

	if (mapInstance.HasWads())
	{
		for (const auto& wf : mapInstance.Wads())
		{
			wadMgr.AddWadFile(opts.wadPath + wf);
		}
	}
	mapInstance.LoadTextures([this](std::string name) -> qformats::textures::ITexture*
	{
	  return this->onTextureRequest(name);
	});

	auto textures = mapInstance.GetTextures();

	defaultMaterial = LoadMaterialDefault();
	defaultMaterial.maps[MATERIAL_MAP_DIFFUSE].color = opts.defaultColor;

	materialPool = (Material*)MemAlloc(mapInstance.GetTextures().size() * sizeof(Material));
	for (int i = 0; i < textures.size(); i++)
	{
		materialPool[i] = ((qformats::textures::Texture<Material>*)(textures[i]))->Data();
	}
	size_t clippedFacesTotal = 0;
	for (const auto& se : mapInstance.GetSolidEntities())
	{
		auto m = readModelEntity(se);
		clippedFacesTotal += se->StatsClippedFaces();
		models.push_back(m);
	}

	std::cout << "faces clipped: " << clippedFacesTotal << std::endl;
}

QuakeModel QuakeMap::readModelEntity(const qformats::map::SolidEntityPtr& ent)
{
	QuakeModel qm;

	for (const auto& b : ent->GetClippedBrushes())
	{
		for (const auto& p : b.GetFaces())
		{
			if (p->noDraw)
			{
				continue;
			}
			p->UpdateNormals();
			int i = 0;
			int i_indices = 0;
			int i_uv = 0;

			auto mesh = Mesh{ 0 };
			auto vertices = p->GetVertices();
			auto indices = p->GetIndices();

			mesh.triangleCount = indices.size() / 3;
			mesh.vertexCount = vertices.size();
			mesh.indices = (ushort*)MemAlloc(indices.size() * sizeof(ushort));
			mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
			mesh.tangents = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
			mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
			mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));

			for (int v = vertices.size() - 1; v >= 0; v--)
			{
				mesh.vertices[i] = -vertices[v].point[0] / opts.inverseScale;
				mesh.tangents[i] = vertices[v].tangent[0];
				mesh.normals[i] = vertices[v].normal[0];
				i++;
				mesh.vertices[i] = vertices[v].point[2] / opts.inverseScale;
				mesh.tangents[i] = vertices[v].tangent[2];
				mesh.normals[i] = -vertices[v].normal[2];
				i++;
				mesh.vertices[i] = vertices[v].point[1] / opts.inverseScale;
				mesh.tangents[i] = vertices[v].tangent[1];
				mesh.normals[i] = -vertices[v].normal[1];

				i++;
				mesh.texcoords[i_uv++] = vertices[v].uv[0];
				mesh.texcoords[i_uv++] = vertices[v].uv[1];
			}
			for (auto idx : indices)
			{
				mesh.indices[i_indices] = idx;
				i_indices++;
			}

			UploadMesh(&mesh, false);
			qm.meshes.push_back(mesh);
			qm.materialIDs.push_back(p->TextureID());
		}
	}

	qm.model.transform = MatrixIdentity();
	qm.model.meshCount = qm.meshes.size();
	qm.model.materialCount = mapInstance.GetTextures().size();
	qm.model.meshes = (Mesh*)MemAlloc(qm.model.meshCount * sizeof(Mesh));
	qm.model.materials = materialPool;
	qm.model.meshMaterial = (int*)MemAlloc(qm.model.meshCount * sizeof(int));

	for (int m = 0; m < qm.meshes.size(); m++)
	{
		qm.model.meshes[m] = qm.meshes[m];
		int matID = qm.materialIDs[m];
		qm.model.meshMaterial[m] = matID;
	}
	return qm;
}

qformats::map::PointEntityPtr QuakeMap::GetPlayerStart()
{
	auto ents = mapInstance.GetPointEntitiesByClass("info_player_start");
	return ents.size() > 0 ? ents[0] : nullptr;
}
QuakeMap::~QuakeMap()
{
	for (auto& m : models)
	{
		for (auto& mesh : m.meshes)
		{
			UnloadMesh(mesh);
		}
		UnloadModel(m.model);
	}
	for (int i = 0; i < mapInstance.GetTextures().size(); i++)
	{
		UnloadTexture(materialPool[i].maps->texture);
		UnloadMaterial(materialPool[i]);

	}
	MemFree(materialPool);
}
