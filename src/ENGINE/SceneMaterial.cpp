#include "SceneMaterial.h"
#include "Game.h"
#include "SpriteManager.h"
#include "Texture.h"
#include "render/ColorPipeline.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>

namespace
{
	// Kept out of the exported class so its layout doesn't change.
	std::string loadedPath = "data/materials.txt";   // the file Load read (SaveChanges writes it)
	std::set<std::string> discarded;                  // made in the editor, then undone

	// A material's maps beyond the normal map, which the exported class has no
	// room for: per library material, by name (extraMaps).
	struct ExtraMaps
	{
		std::string emissivePath;    // `emissivemap`: multiplies `emissive`
		std::string roughnessPath;   // `roughnessmap`: G x roughness, B x metallic
		Texture* emissive = nullptr;
		Texture* roughness = nullptr;
	};
	std::map<std::string, ExtraMaps> extraMaps;

	// A glow map is colour (sRGB, decoded in a linear workflow); a roughness
	// map is data.
	Texture* LoadEmissiveMap(Game& game, const std::string& path)
	{
		if (path.empty())
			return nullptr;
		Texture* t = game.spriteManager.GetImage(path, Texture::Filter::Smooth, LinearWorkflow());
		if (t == nullptr)
			std::cout << "MaterialLibrary: emissive map not found: " << path << std::endl;
		return t;
	}

	Texture* LoadRoughnessMap(Game& game, const std::string& path)
	{
		if (path.empty())
			return nullptr;
		Texture* t = game.spriteManager.GetImage(path, Texture::Filter::Smooth);
		if (t == nullptr)
			std::cout << "MaterialLibrary: roughness map not found: " << path << std::endl;
		return t;
	}

	float ParseNumber(const std::string& s)
	{
		try { return std::stof(s); } catch (...) { return 0.0f; }
	}

	// Apply one field line (`tok` and the rest in `ss`) to m and its extra maps
	// x. False if `tok` isn't a material field.
	bool ApplyField(SceneMaterial& m, ExtraMaps& x, const std::string& tok, std::istringstream& ss)
	{
		std::string v;
		if (tok == "lighting")
		{
			ss >> v;
			m.lighting = (v == "pbr") ? LightingModel::PBR : (v == "water") ? LightingModel::Water : LightingModel::Phong;
		}
		else if (tok == "tint")           { ss >> m.tint.x >> m.tint.y >> m.tint.z; }
		else if (tok == "specular")       { ss >> v; m.specular = ParseNumber(v); }
		else if (tok == "shininess")      { ss >> v; m.shininess = ParseNumber(v); }
		else if (tok == "emissive")       { ss >> m.emissive.x >> m.emissive.y >> m.emissive.z; }
		else if (tok == "fresnel")        { ss >> v; m.fresnel = ParseNumber(v); }
		else if (tok == "uvtile")         { ss >> m.uvTile.x >> m.uvTile.y; }
		else if (tok == "normal")         { ss >> m.normalMapPath; }
		else if (tok == "normalstrength") { ss >> v; m.normalStrength = ParseNumber(v); }
		else if (tok == "normalmode")     { ss >> v; m.normalMode = (v == "vertex") ? NormalMode::Vertex : NormalMode::ScreenSpace; }
		else if (tok == "emissivemap")    { ss >> x.emissivePath; }
		else if (tok == "roughnessmap")   { ss >> x.roughnessPath; }
		else if (tok == "metallic")       { ss >> v; m.metallic = ParseNumber(v); }
		else if (tok == "roughness")      { ss >> v; m.roughness = ParseNumber(v); }
		else if (tok == "opacity")        { ss >> v; m.opacity = ParseNumber(v); }
		else if (tok == "outline")        { ss >> v; m.outline = !(v == "off" || v == "0" || v == "false"); }
		else if (tok == "seasonal")       { m.seasonal = true; }
		else if (tok == "deciduous")      { m.deciduous = true; m.seasonal = true; }
		else
			return false;
		return true;
	}

	struct Parsed
	{
		SceneMaterial material;
		ExtraMaps maps;
	};

	// The materials in a material file (or the editor's snapshot text).
	std::vector<Parsed> ParseMaterials(std::istream& in)
	{
		std::vector<Parsed> out;
		Parsed cur;
		bool have = false;
		std::string line;
		while (std::getline(in, line))
		{
			std::istringstream ss(line);
			std::string tok;
			if (!(ss >> tok) || tok[0] == '#' || tok[0] == ';')
				continue;
			if (tok == "material")
			{
				if (have)
					out.push_back(cur);
				cur = Parsed();
				ss >> cur.material.name;
				have = true;
			}
			else if (have)
			{
				ApplyField(cur.material, cur.maps, tok, ss);
			}
		}
		if (have)
			out.push_back(cur);
		return out;
	}

	std::string Num(float v)
	{
		std::ostringstream ss;
		ss << v;
		return ss.str();
	}

	// The fields a material is written with, in file order. "season" stands for
	// the seasonal / deciduous flag lines.
	const char* const kFields[] = { "lighting", "specular", "shininess", "metallic", "roughness", "roughnessmap",
		"tint", "emissive", "emissivemap", "fresnel", "uvtile", "normal", "normalstrength", "normalmode", "opacity",
		"outline", "season" };

	// A field's value as written after its keyword ("" = no line: an unset
	// map, or no season flag).
	std::string FieldValue(const SceneMaterial& m, const ExtraMaps& x, const std::string& f)
	{
		if (f == "lighting") return m.lighting == LightingModel::PBR ? "pbr" : m.lighting == LightingModel::Water ? "water" : "phong";
		if (f == "specular") return Num(m.specular);
		if (f == "shininess") return Num(m.shininess);
		if (f == "metallic") return Num(m.metallic);
		if (f == "roughness") return Num(m.roughness);
		if (f == "tint") return Num(m.tint.x) + " " + Num(m.tint.y) + " " + Num(m.tint.z);
		if (f == "emissive") return Num(m.emissive.x) + " " + Num(m.emissive.y) + " " + Num(m.emissive.z);
		if (f == "fresnel") return Num(m.fresnel);
		if (f == "uvtile") return Num(m.uvTile.x) + " " + Num(m.uvTile.y);
		if (f == "normal") return m.normalMapPath;
		if (f == "emissivemap") return x.emissivePath;
		if (f == "roughnessmap") return x.roughnessPath;
		if (f == "normalstrength") return Num(m.normalStrength);
		if (f == "normalmode") return m.normalMode == NormalMode::Vertex ? "vertex" : "screen";
		if (f == "opacity") return Num(m.opacity);
		if (f == "outline") return m.outline ? "on" : "off";
		if (f == "season") return m.deciduous ? "deciduous" : m.seasonal ? "seasonal" : "";
		return "";
	}

	// The line(s) a field is written as: "season" is a bare keyword.
	std::string FieldLine(const std::string& f, const std::string& value)
	{
		return (f == "season") ? value : f + " " + value;
	}

	bool IsSeasonToken(const std::string& tok)
	{
		return tok == "seasonal" || tok == "deciduous";
	}
}

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
		if (discarded.count(m.name) == 0)
			out.push_back(m.name);
	return out;
}

SceneMaterial* MaterialLibrary::FindMutable(const std::string& name)
{
	auto it = byName.find(name);
	return (it == byName.end()) ? nullptr : &materials[it->second];
}

void MaterialLibrary::SetNormalMap(Game& game, SceneMaterial& material, const std::string& path)
{
	material.normalMapPath = path;
	material.normalMap = path.empty() ? nullptr : game.spriteManager.GetImage(path, Texture::Filter::Smooth);
	if (!path.empty() && material.normalMap == nullptr)
		std::cout << "MaterialLibrary: normal map not found: " << path << std::endl;
}

void MaterialLibrary::SetEmissiveMap(Game& game, SceneMaterial& material, const std::string& path)
{
	ExtraMaps& x = extraMaps[material.name];
	x.emissivePath = path;
	x.emissive = LoadEmissiveMap(game, path);
}

void MaterialLibrary::SetRoughnessMap(Game& game, SceneMaterial& material, const std::string& path)
{
	ExtraMaps& x = extraMaps[material.name];
	x.roughnessPath = path;
	x.roughness = LoadRoughnessMap(game, path);
}

std::string MaterialLibrary::EmissiveMapPath(const SceneMaterial& material) const
{
	auto it = extraMaps.find(material.name);
	return (it == extraMaps.end()) ? std::string() : it->second.emissivePath;
}

std::string MaterialLibrary::RoughnessMapPath(const SceneMaterial& material) const
{
	auto it = extraMaps.find(material.name);
	return (it == extraMaps.end()) ? std::string() : it->second.roughnessPath;
}

Texture* MaterialLibrary::EmissiveMap(const SceneMaterial& material) const
{
	if (Find(material.name) != &material)   // a model's own material, or a copy
		return nullptr;
	auto it = extraMaps.find(material.name);
	return (it == extraMaps.end()) ? nullptr : it->second.emissive;
}

Texture* MaterialLibrary::RoughnessMap(const SceneMaterial& material) const
{
	if (Find(material.name) != &material)
		return nullptr;
	auto it = extraMaps.find(material.name);
	return (it == extraMaps.end()) ? nullptr : it->second.roughness;
}

SceneMaterial* MaterialLibrary::Add(Game& game, const SceneMaterial& material)
{
	discarded.erase(material.name);
	SceneMaterial* m = FindMutable(material.name);
	if (m == nullptr)
	{
		materials.push_back(material);
		byName[material.name] = (int)materials.size() - 1;
		m = &materials.back();
	}
	else
	{
		*m = material;
	}
	SetNormalMap(game, *m, material.normalMapPath);
	return m;
}

const std::string& MaterialLibrary::LoadedPath() const
{
	return loadedPath;
}

std::string MaterialLibrary::Serialize() const
{
	std::string out;
	for (const SceneMaterial& m : materials)
	{
		if (discarded.count(m.name) != 0)
			continue;
		out += "material " + m.name + "\n";
		auto x = extraMaps.find(m.name);
		const ExtraMaps maps = (x == extraMaps.end()) ? ExtraMaps() : x->second;
		for (const char* f : kFields)
		{
			const std::string v = FieldValue(m, maps, f);
			if (!v.empty())
				out += FieldLine(f, v) + "\n";
		}
	}
	return out;
}

void MaterialLibrary::ApplySerialized(Game& game, const std::string& text)
{
	std::istringstream in(text);
	const std::vector<Parsed> snapshot = ParseMaterials(in);
	std::set<std::string> present;
	for (const Parsed& s : snapshot)
	{
		present.insert(s.material.name);
		SceneMaterial* m = Add(game, s.material);
		if (EmissiveMapPath(*m) != s.maps.emissivePath)
			SetEmissiveMap(game, *m, s.maps.emissivePath);
		if (RoughnessMapPath(*m) != s.maps.roughnessPath)
			SetRoughnessMap(game, *m, s.maps.roughnessPath);
	}
	for (const SceneMaterial& m : materials)
		if (present.count(m.name) == 0)
			discarded.insert(m.name);
}

bool MaterialLibrary::SaveChanges(std::string& message) const
{
	// The file as it is now, line by line.
	// Each line keeps its own ending (files can mix CRLF and LF); added lines
	// take their neighbour's.
	std::vector<std::string> lines;
	std::vector<bool> crlf;
	{
		std::ifstream in(loadedPath, std::ios::binary);
		std::string line;
		while (std::getline(in, line))
		{
			const bool cr = !line.empty() && line.back() == '\r';
			if (cr)
				line.pop_back();
			lines.push_back(line);
			crlf.push_back(cr);
		}
	}

	// Its blocks: the header line, and the field lines up to the next header.
	struct Block { std::string name; int header; std::vector<int> fieldLines; };
	std::vector<Block> blocks;
	for (int i = 0; i < (int)lines.size(); i++)
	{
		std::istringstream ss(lines[i]);
		std::string tok;
		if (!(ss >> tok) || tok[0] == '#' || tok[0] == ';')
			continue;
		if (tok == "material")
		{
			Block b;
			ss >> b.name;
			b.header = i;
			blocks.push_back(b);
		}
		else if (!blocks.empty())
		{
			SceneMaterial probe;
			ExtraMaps probeMaps;
			std::istringstream rest(lines[i]);
			std::string t;
			rest >> t;
			if (ApplyField(probe, probeMaps, t, rest))
				blocks.back().fieldLines.push_back(i);
		}
	}

	// Per line: a replacement, or removal; per line index: lines to insert after it.
	std::vector<bool> removed(lines.size(), false);
	std::vector<std::vector<std::string>> insertAfter(lines.size());
	std::vector<std::string> appended;
	const SceneMaterial defaults;
	const ExtraMaps noMaps;
	int changedMaterials = 0;

	for (const SceneMaterial& m : materials)
	{
		if (discarded.count(m.name) != 0)
			continue;
		auto x = extraMaps.find(m.name);
		const ExtraMaps maps = (x == extraMaps.end()) ? ExtraMaps() : x->second;
		const Block* block = nullptr;
		for (const Block& b : blocks)
			if (b.name == m.name)
				block = &b;

		if (block == nullptr)
		{
			// A new material: its own block at the end, non-default fields only.
			appended.push_back("");
			appended.push_back("material " + m.name);
			for (const char* f : kFields)
			{
				const std::string v = FieldValue(m, maps, f);
				if (!v.empty() && (v != FieldValue(defaults, noMaps, f) || std::string(f) == "lighting"))
					appended.push_back(FieldLine(f, v));
			}
			changedMaterials++;
			continue;
		}

		// What the file says now, field by field.
		SceneMaterial fileMat;
		ExtraMaps fileMaps;
		for (int li : block->fieldLines)
		{
			std::istringstream ss(lines[li]);
			std::string tok;
			ss >> tok;
			ApplyField(fileMat, fileMaps, tok, ss);
		}
		const int insertAt = block->fieldLines.empty() ? block->header : block->fieldLines.back();
		bool changed = false;
		for (const char* f : kFields)
		{
			const std::string field = f;
			const std::string want = FieldValue(m, maps, field);
			if (want == FieldValue(fileMat, fileMaps, field))
				continue;
			changed = true;
			// The lines that set this field now (the last one wins on load).
			std::vector<int> existing;
			for (int li : block->fieldLines)
			{
				std::istringstream ss(lines[li]);
				std::string tok;
				ss >> tok;
				if (field == "season" ? IsSeasonToken(tok) : tok == field)
					existing.push_back(li);
			}
			if (want.empty())   // no map / no season flag: drop the lines
			{
				for (int li : existing)
					removed[li] = true;
			}
			else if (!existing.empty() && field != "season")
			{
				for (size_t k = 0; k + 1 < existing.size(); k++)
					removed[existing[k]] = true;
				lines[existing.back()] = FieldLine(field, want);
			}
			else
			{
				for (int li : existing)
					removed[li] = true;
				insertAfter[insertAt].push_back(FieldLine(field, want));
			}
		}
		if (changed)
			changedMaterials++;
	}

	if (changedMaterials == 0)
	{
		message = "materials unchanged";
		return true;
	}

	std::ofstream out(loadedPath, std::ios::binary | std::ios::trunc);
	if (!out.is_open())
	{
		message = "cannot write " + loadedPath;
		return false;
	}
	auto eol = [&](size_t i) { return (i < crlf.size() && crlf[i]) ? "\r\n" : "\n"; };
	if (lines.empty())
		out << "# Material library for 3D scenes (see SceneMaterial.h).\n";
	for (size_t i = 0; i < lines.size(); i++)
	{
		if (!removed[i])
			out << lines[i] << eol(i);
		for (const std::string& add : insertAfter[i])
			out << add << eol(i);
	}
	for (const std::string& add : appended)
		out << add << eol(lines.empty() ? 0 : lines.size() - 1);
	message = std::to_string(changedMaterials) + (changedMaterials == 1 ? " material" : " materials") + " saved to " + loadedPath;
	return true;
}

bool MaterialLibrary::Load(Game& game, const std::string& path)
{
	materials.clear();
	byName.clear();
	discarded.clear();
	extraMaps.clear();
	loadedPath = path;

	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cout << "MaterialLibrary: no " << path << " (using default material only)" << std::endl;
		return false;
	}
	const std::vector<Parsed> parsed = ParseMaterials(file);
	// Room for materials the editor adds, so the list rarely moves in memory.
	materials.reserve(parsed.size() + 64);
	for (const Parsed& p : parsed)
		materials.push_back(p.material);

	// Resolve the maps' textures and build the name lookup.
	for (size_t i = 0; i < materials.size(); i++)
	{
		SceneMaterial& m = materials[i];
		const ExtraMaps& x = parsed[i].maps;
		if (!x.emissivePath.empty() || !x.roughnessPath.empty())
		{
			ExtraMaps& own = extraMaps[m.name];
			own.emissivePath = x.emissivePath;
			own.roughnessPath = x.roughnessPath;
			own.emissive = LoadEmissiveMap(game, x.emissivePath);
			own.roughness = LoadRoughnessMap(game, x.roughnessPath);
		}
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
