#include "QuakeMap.h"
#include <raymath.h>
#include <raylib.h>

#include <iostream>
#include <locale>

#define LIGHTMAPPER_IMPLEMENTATION
#define LM_DEBUG_INTERPOLATION
#include <OpenGL/gl3.h>
#include "thirdparty/lightmapper.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

string to_lower(string s)
{
    for (char &c : s)
        c = tolower(c);
    return s;
}

qformats::textures::ITexture *QuakeMap::onTextureRequest(std::string name)
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
    Vector3 position = {0.0f, 0.0f, 0.0f};
    for (const auto &m : models)
    {
        DrawModel(m.model, position, 1, WHITE);
        break;
    }
}

void QuakeMap::LoadMapFromFile(std::string fileName)
{

    mapInstance = qformats::map::QMap();
    mapInstance.LoadFile(fileName);
    mapInstance.GenerateGeometry();

    if (mapInstance.HasWads())
    {
        for (const auto &wf : mapInstance.Wads())
        {
            wadMgr.AddWadFile(opts.wadPath + wf);
        }
    }
    mapInstance.LoadTextures([this](std::string name) -> qformats::textures::ITexture *
                             { return this->onTextureRequest(name); });

    auto textures = mapInstance.GetTextures();

    defaultMaterial = LoadMaterialDefault();
    defaultMaterial.maps[MATERIAL_MAP_DIFFUSE].color = opts.defaultColor;

    materialPool = (Material *)MemAlloc(mapInstance.GetTextures().size() * sizeof(Material));
    for (int i = 0; i < textures.size(); i++)
    {
        materialPool[i] = ((qformats::textures::Texture<Material> *)(textures[i]))->Data();
    }
    for (const auto &se : mapInstance.GetSolidEntities())
    {
        auto m = readModelEntity(se);
        models.push_back(m);
    }
}

QuakeModel QuakeMap::readModelEntity(const qformats::map::SolidEntity &ent)
{
    QuakeModel qm;
    auto atlas = xatlas::Create();

    for (const auto &b : ent.geoBrushes)
    {
        for (const auto &p : b.polygons)
        {
            p->RecalcNormals();
            int i = 0;
            int i_indices = 0;
            int i_uv = 0;

            auto mesh = Mesh{0};
            mesh.triangleCount = p->indices.size() / 3;
            mesh.vertexCount = p->vertices.size();
            mesh.indices = (ushort *)MemAlloc(p->indices.size() * sizeof(ushort));
            mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float)); // 3 vertices, 3 coordinates each (x, y, z)
            mesh.tangents = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float)); // 3 vertices, 3 coordinates each (x, y, z)
            mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
            mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float)); // 3 vertices, 3 coordinates each (x, y, z)

            auto atlasMeshDecl = xatlas::MeshDecl();

            for (int v = p->vertices.size() - 1; v >= 0; v--)
            {
                mesh.vertices[i] = -p->vertices[v].point[0] / opts.inverseScale;
                mesh.tangents[i] = p->vertices[v].tangent[0];
                mesh.normals[i] = p->vertices[v].normal[0];
                i++;
                mesh.vertices[i] = p->vertices[v].point[2] / opts.inverseScale;
                mesh.tangents[i] = p->vertices[v].tangent[2];
                mesh.normals[i] = -p->vertices[v].normal[2];
                i++;
                mesh.vertices[i] = p->vertices[v].point[1] / opts.inverseScale;
                mesh.tangents[i] = p->vertices[v].tangent[1];
                mesh.normals[i] = -p->vertices[v].normal[1];

                i++;
                mesh.texcoords[i_uv++] = p->vertices[v].uv[0];
                mesh.texcoords[i_uv++] = p->vertices[v].uv[1];
            }
            for (auto idx : p->indices)
            {
                mesh.indices[i_indices] = idx;
                i_indices++;
            }

            UploadMesh(&mesh, false);
            qm.meshes.push_back(mesh);
            qm.materialIDs.push_back(p->faceRef.textureID);
        }
    }

    qm.model.transform = MatrixIdentity();
    qm.model.meshCount = qm.meshes.size();
    qm.model.materialCount = mapInstance.GetTextures().size();
    qm.model.meshes = (Mesh *)MemAlloc(qm.model.meshCount * sizeof(Mesh));
    qm.model.materials = materialPool;
    qm.model.meshMaterial = (int *)MemAlloc(qm.model.meshCount * sizeof(int));

    for (int m = 0; m < qm.meshes.size(); m++)
    {
        qm.model.meshes[m] = qm.meshes[m];
        int matID = qm.materialIDs[m];
        qm.model.meshMaterial[m] = matID;
    }

    return qm;
}

qformats::map::QPointEntity *QuakeMap::GetPlayerStart()
{
    auto ents = mapInstance.GetPointEntitiesByClass("info_player_start");
    return ents.size() > 0 ? ents[0] : nullptr;
}