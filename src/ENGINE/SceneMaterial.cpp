#include "SceneMaterial.h"
#include "Game.h"
#include "SpriteManager.h"
#include "Texture.h"
#include <fstream>
#include <sstream>
#include <iostream>

MaterialLibrary& MaterialLibrary::Get()
{
	static MaterialLibrary instance;
	return instance;
}

const SceneMaterial* MaterialLibrary::Find(const std::string& name) const
{
	auto it = byName.find(name);
	if (it == byName.end())
		return nullptr;
	return &materials[it->second];
}

std::vector<std::string> MaterialLibrary::Names() const
{
	std::vector<std::string> out;
	for (const SceneMaterial& m : materials)
		out.push_back(m.name);
	return out;
}

bool MaterialLibrary::Load(Game& game, const std::string& path)
{
	materials.clear();
	byName.clear();

	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cout << "MaterialLibrary: no " << path << " (using default material only)" << std::endl;
		return false;
	}

	auto pf = [](const std::string& s) -> float
	{
		try { return std::stof(s); } catch (...) { return 0.0f; }
	};

	SceneMaterial cur;
	bool have = false;
	auto flush = [&]()
	{
		if (have)
			materials.push_back(cur);
		have = false;
	};

	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream ss(line);
		std::string tok;
		if (!(ss >> tok))
			continue;
		if (tok.empty() || tok[0] == '#' || tok[0] == ';')
			continue;

		if (tok == "material")
		{
			flush();
			cur = SceneMaterial();
			ss >> cur.name;
			have = true;
		}
		else if (!have)
		{
			continue;   // stray field before any "material"
		}
		else if (tok == "lighting")
		{
			std::string v; ss >> v;
			cur.lighting = (v == "pbr")   ? LightingModel::PBR
			             : (v == "water") ? LightingModel::Water
			             :                  LightingModel::Phong;
		}
		else if (tok == "tint")       { ss >> cur.tint.x >> cur.tint.y >> cur.tint.z; }
		else if (tok == "specular")   { std::string v; ss >> v; cur.specular = pf(v); }
		else if (tok == "shininess")  { std::string v; ss >> v; cur.shininess = pf(v); }
		else if (tok == "emissive")   { ss >> cur.emissive.x >> cur.emissive.y >> cur.emissive.z; }
		else if (tok == "fresnel")    { std::string v; ss >> v; cur.fresnel = pf(v); }
		else if (tok == "uvtile")     { ss >> cur.uvTile.x >> cur.uvTile.y; }
		else if (tok == "normal")     { ss >> cur.normalMapPath; }
		else if (tok == "normalstrength") { std::string v; ss >> v; cur.normalStrength = pf(v); }
		else if (tok == "normalmode") { std::string v; ss >> v; cur.normalMode = (v == "vertex") ? NormalMode::Vertex : NormalMode::ScreenSpace; }
		else if (tok == "metallic")   { std::string v; ss >> v; cur.metallic = pf(v); }
		else if (tok == "roughness")  { std::string v; ss >> v; cur.roughness = pf(v); }
		else if (tok == "opacity")    { std::string v; ss >> v; cur.opacity = pf(v); }
		else if (tok == "outline")    { std::string v; ss >> v; cur.outline = !(v == "off" || v == "0" || v == "false"); }
	}
	flush();

	// Resolve normal-map textures and build the name lookup.
	for (size_t i = 0; i < materials.size(); i++)
	{
		SceneMaterial& m = materials[i];
		if (!m.normalMapPath.empty())
		{
			m.normalMap = game.spriteManager.GetImage(m.normalMapPath, Texture::Filter::Smooth);
			if (m.normalMap == nullptr)
				std::cout << "MaterialLibrary: normal map not found: " << m.normalMapPath << std::endl;
		}
		byName[m.name] = (int)i;
	}

	std::cout << "MaterialLibrary: loaded " << materials.size() << " materials" << std::endl;
	return true;
}
