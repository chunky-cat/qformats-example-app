#pragma once

#include <raylib.h>
#include <qformats/map/map.h>
#include <qformats/wad/wad.h>
#include <string>
#include <vector>
#include <map>

struct QuakeModel
{
	Model model;
	std::vector<Mesh> meshes;
	std::vector<int> materialIDs;
};

struct QuakeMapOptions
{
	std::string texturePath = "textures/";
	std::string wadPath = "wads/";
	float inverseScale = 24.0f;
	Shader shader;
	bool showGrid = false;
	bool showVerts = false;
	bool wireframe = false;
	Color backgroundColor = WHITE;
	Color defaultColor = WHITE;
};

class QuakeMap
{
public:
	QuakeMap(QuakeMapOptions opts)
			:opts(opts)
	{
	};

	~QuakeMap();

	void DrawQuakeSolids();
	qformats::map::PointEntityPtr GetPlayerStart();
	void LoadMapFromFile(std::string fileName);
	const QuakeMapOptions& Opts()
	{
		return opts;
	}

private:
	QuakeModel readModelEntity(const qformats::map::SolidEntityPtr& ent);
	qformats::textures::ITexture* onTextureRequest(std::string name);
	Texture2D getTextureFromWad(std::string name);
	QuakeMapOptions opts;
	qformats::map::QMap mapInstance;
	qformats::wad::QuakeWadManager wadMgr;
	Material* materialPool;
	std::vector<QuakeModel> models;
	Material defaultMaterial;
};