#include "QuakeMap.h"
#include <raymath.h>
#include <raylib.h>

#include <iostream>
#include <locale> // std::locale, std::tolower

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

    for (const auto &se : mapInstance.solidEntities)
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
            mesh.texcoords2 = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
            mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float)); // 3 vertices, 3 coordinates each (x, y, z)

            auto atlasMeshDecl = xatlas::MeshDecl();

            for (const auto &v : p->vertices)
            {
                mesh.vertices[i] = v.point.x / opts.inverseScale;
                mesh.tangents[i] = v.tangent.x;
                mesh.normals[i] = v.normal.x;
                i++;
                mesh.vertices[i] = v.point.z / opts.inverseScale;
                mesh.tangents[i] = v.tangent.z;
                mesh.normals[i] = v.normal.z;
                i++;
                mesh.vertices[i] = v.point.y / opts.inverseScale;
                mesh.tangents[i] = v.tangent.y;
                mesh.normals[i] = v.normal.y;

                i++;
                mesh.texcoords[i_uv++] = v.uv.x;
                mesh.texcoords[i_uv++] = v.uv.y;
            }
            for (auto idx : p->indices)
            {
                mesh.indices[i_indices] = idx;
                i_indices++;
            }

            atlasMeshDecl.vertexPositionData = mesh.vertices;
            atlasMeshDecl.vertexPositionStride = 3 * sizeof(float);
            atlasMeshDecl.vertexUvData = mesh.texcoords;
            atlasMeshDecl.vertexUvStride = 2 * sizeof(float);
            atlasMeshDecl.vertexCount = mesh.vertexCount;
            atlasMeshDecl.indexCount = p->indices.size();
            atlasMeshDecl.indexFormat = xatlas::IndexFormat::UInt16;
            atlasMeshDecl.indexData = mesh.indices;

            auto err = xatlas::AddMesh(atlas, atlasMeshDecl, qm.meshes.size());

            qm.meshes.push_back(mesh);
            qm.materialIDs.push_back(p->faceRef.textureID);
        }
    }

    xatlas::Generate(atlas);

    for (uint32_t i = 0; i < atlas->meshCount; i++)
    {
        const xatlas::Mesh &atlasMesh = atlas->meshes[i];
        int i_uv = 0;
        for (int j = 0; j < atlasMesh.vertexCount; j++)
        {
            if (atlasMesh.vertexArray[j].atlasIndex < 0)
                continue;
            const xatlas::Vertex &atlasVert = atlasMesh.vertexArray[j];
            qm.meshes[i].texcoords2[i_uv++] = atlasVert.uv[0];
            qm.meshes[i].texcoords2[i_uv++] = atlasVert.uv[1];
        }
        UploadMesh(&qm.meshes[i], false);
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