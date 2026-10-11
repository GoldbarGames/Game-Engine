#include "SceneMaterial.h"
#include "Game.h"
#include "Scene3D.h"
#include "Scene3DInternal.h"
#include "Shader.h"
#include "SpriteManager.h"
#include "StartupTrace.h"
#include "Texture.h"
#include "render/ColorPipeline.h"
#include "render/RenderDevice.h"
#include "render/TextureFiles.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include <thread>
#include <unordered_map>

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
		std::string splatPath;       // `splat`: up to three ground layers, space-separated
		float wind = 0.0f;           // `wind`: sway in a fresh breeze, world units (Wind.h)...
		float windHeight = 0.0f;     // ...weighted by this height (0: by the mesh's tangent slot)
		float windFlutter = 1.0f;    // ...and its flutter scaled by this (bark 0, leaves 1)
		float translucency = 0.0f;   // `translucency`: sunlight through leaves from behind
		float cutout = 0.0f;         // `cutout`: alpha cut-out threshold (0: the usual 0.1)
		bool fade = false;           // `fade on`: dissolves near the occluder-fade line
		bool noShadow = false;       // `shadow off`: casts none (grass: too small to show, too many to draw)
		bool forceShadow = false;    // `shadow on`: casts, however small (a golf ball, a flagstick)

		// Anything beyond the defaults (worth an entry in extraMaps)
		bool Any() const
		{
			return !emissivePath.empty() || !roughnessPath.empty() || !splatPath.empty() || wind != 0.0f
				|| windFlutter != 1.0f || translucency != 0.0f || cutout != 0.0f || fade || noShadow || forceShadow;
		}
		Texture* emissive = nullptr;
		Texture* roughness = nullptr;
		unsigned int splatTexture = 0;   // the layers as one texture array, made on first use...
		int splatSeason = -1;            // ...for this season (Scene3D::Season)
	};
	std::map<std::string, ExtraMaps> extraMaps;

	// The library's own materials' extra fields by address, for the accessors
	// the renderer calls per model and per draw: by name, two string-keyed
	// lookups each were several milliseconds a frame in a Debug build with a
	// thousand models (Golf Galaxy's trees). Rebuilt after any change to the
	// library (ExtrasChanged), with the same test as before: a material is the
	// library's own only if Find(its name) is it.
	std::unordered_map<const SceneMaterial*, ExtraMaps*> extraByAddress;
	bool extraByAddressValid = false;

	unsigned int libraryVersion = 0;   // Scene3DInternal::MaterialLibraryVersion

	void ExtrasChanged()
	{
		extraByAddressValid = false;
		libraryVersion++;
	}

	// The extra fields of one of the library's own materials (nullptr: none,
	// or a model's own material, or a copy)
	ExtraMaps* OwnExtras(const MaterialLibrary& library, const SceneMaterial& material)
	{
		if (extraMaps.empty())
			return nullptr;
		if (!extraByAddressValid)
		{
			extraByAddress.clear();
			for (const SceneMaterial& m : library.All())
			{
				if (library.Find(m.name) != &m)
					continue;
				auto it = extraMaps.find(m.name);
				if (it != extraMaps.end())
					extraByAddress[&m] = &it->second;
			}
			extraByAddressValid = true;
		}
		auto it = extraByAddress.find(&material);
		return it == extraByAddress.end() ? nullptr : it->second;
	}

	// Splat layers sample as one texture array: three layers, all the size of
	// the first, one after another. A missing layer is mid grey (its weights
	// should be nought anyway); a layer of another size is stretched to fit.
	// `suffix` picks a season's variant ("_winter") where one exists.
	unsigned int BuildSplatArray(const std::string& paths, const char* suffix)
	{
		const int LAYERS = 3;
		std::vector<std::string> files;
		std::istringstream ss(paths);
		for (std::string p; ss >> p && (int)files.size() < LAYERS;)
		{
			if (suffix[0] != '\0')
			{
				const size_t dot = p.find_last_of('.');
				const std::string variant = (dot == std::string::npos) ? p + suffix
					: p.substr(0, dot) + suffix + p.substr(dot);
				if (std::ifstream(variant).good())
					p = variant;
			}
			files.push_back(p);
		}
		if (files.empty())
			return 0;

		// Decoded side by side: three large PNGs one after another were most of
		// a first frame. (SDL_image's PNG support is already up by now - the
		// call here makes sure of it before any thread asks.)
		IMG_Init(IMG_INIT_PNG);
		std::vector<SDL_Surface*> layers(files.size(), nullptr);
		{
			std::vector<std::thread> decoders;
			for (size_t i = 0; i < files.size(); i++)
			{
				decoders.emplace_back([&files, &layers, i]()
				{
					SDL_Surface* raw = IMG_Load(files[i].c_str());
					if (raw != nullptr)
					{
						layers[i] = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
						SDL_FreeSurface(raw);
					}
				});
			}
			for (std::thread& t : decoders)
				t.join();
		}
		int w = 0, h = 0;
		for (size_t i = 0; i < files.size(); i++)
		{
			if (layers[i] == nullptr)
				std::cout << "MaterialLibrary: splat layer not found: " << files[i] << std::endl;
			else if (w == 0)
			{
				w = layers[i]->w;
				h = layers[i]->h;
			}
		}
		if (w == 0)
			return 0;

		std::vector<unsigned char> pixels((size_t)w * h * 4 * LAYERS, 128);
		for (int i = 0; i < (int)layers.size(); i++)
		{
			SDL_Surface* s = layers[i];
			if (s == nullptr)
				continue;
			if (s->w != w || s->h != h)
			{
				std::cout << "MaterialLibrary: splat layer " << files[i] << " is " << s->w << "x" << s->h
					<< ", not " << w << "x" << h << " like the first: stretched" << std::endl;
				SDL_Surface* fit = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
				SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);
				SDL_BlitScaled(s, nullptr, fit, nullptr);
				SDL_FreeSurface(s);
				s = fit;
			}
			unsigned char* out = pixels.data() + (size_t)w * h * 4 * i;
			for (int y = 0; y < h; y++)
				memcpy(out + (size_t)y * w * 4, (const unsigned char*)s->pixels + (size_t)y * s->pitch, (size_t)w * 4);
			SDL_FreeSurface(s);
		}

		TextureDesc desc;
		desc.type = TextureType::Tex2DArray;
		desc.format = LinearWorkflow() ? TextureFormat::SRGB8_A8 : TextureFormat::RGBA8;   // colour, like an albedo
		desc.width = w;
		desc.height = h;
		desc.layers = LAYERS;
		desc.filter = TextureFilter::Trilinear;
		desc.wrap = TextureWrap::Repeat;
		desc.maxAnisotropy = TextureAnisotropy();
		desc.generateMipmaps = true;
		return Device().CreateTexture(desc, pixels.data()).id;
	}

	void DropSplatArray(ExtraMaps& x)
	{
		if (x.splatTexture != 0)
		{
			TextureHandle t(x.splatTexture);
			Device().DestroyTexture(t);
		}
		x.splatTexture = 0;
		x.splatSeason = -1;
	}

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
		else if (tok == "splat")
		{
			x.splatPath.clear();
			for (std::string p; ss >> p;)
				x.splatPath += (x.splatPath.empty() ? "" : " ") + p;
		}
		else if (tok == "wind")
		{
			ss >> v;
			x.wind = ParseNumber(v);
			x.windHeight = (ss >> v) ? ParseNumber(v) : 0.0f;
			x.windFlutter = (ss >> v) ? ParseNumber(v) : 1.0f;
		}
		else if (tok == "translucency")   { ss >> v; x.translucency = ParseNumber(v); }
		else if (tok == "cutout")         { ss >> v; x.cutout = ParseNumber(v); }
		else if (tok == "fade")           { ss >> v; x.fade = (v == "on" || v == "1" || v == "true"); }
		else if (tok == "shadow")
		{
			ss >> v;
			x.noShadow = (v == "off" || v == "0" || v == "false");
			x.forceShadow = (v == "on" || v == "1" || v == "true");
		}
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
		"outline", "splat", "wind", "translucency", "cutout", "fade", "shadow", "season" };

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
		if (f == "splat") return x.splatPath;
		if (f == "shadow") return x.noShadow ? "off" : x.forceShadow ? "on" : "";
		if (f == "wind")
		{
			if (x.wind == 0.0f)
				return std::string();
			if (x.windFlutter != 1.0f)
				return Num(x.wind) + " " + Num(x.windHeight) + " " + Num(x.windFlutter);
			return Num(x.wind) + (x.windHeight > 0.0f ? " " + Num(x.windHeight) : "");
		}
		if (f == "translucency") return (x.translucency == 0.0f) ? std::string() : Num(x.translucency);
		if (f == "cutout") return (x.cutout == 0.0f) ? std::string() : Num(x.cutout);
		if (f == "fade") return x.fade ? "on" : "";
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
	ExtrasChanged();   // (the list may move, or a caller change it)
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
	ExtrasChanged();
	ExtraMaps& x = extraMaps[material.name];
	x.emissivePath = path;
	x.emissive = LoadEmissiveMap(game, path);
}

void MaterialLibrary::SetRoughnessMap(Game& game, SceneMaterial& material, const std::string& path)
{
	ExtrasChanged();
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
	const ExtraMaps* x = OwnExtras(*this, material);   // (none for a model's own material, or a copy)
	return x != nullptr ? x->emissive : nullptr;
}

Texture* MaterialLibrary::RoughnessMap(const SceneMaterial& material) const
{
	const ExtraMaps* x = OwnExtras(*this, material);
	return x != nullptr ? x->roughness : nullptr;
}

void MaterialLibrary::SetSplatLayers(Game& game, SceneMaterial& material, const std::string& paths)
{
	ExtrasChanged();
	(void)game;
	ExtraMaps& x = extraMaps[material.name];
	DropSplatArray(x);   // made again, from these, on next use
	x.splatPath = paths;
}

std::string MaterialLibrary::SplatLayersPath(const SceneMaterial& material) const
{
	auto it = extraMaps.find(material.name);
	return (it == extraMaps.end()) ? std::string() : it->second.splatPath;
}

unsigned int MaterialLibrary::SplatLayersTexture(const SceneMaterial& material) const
{
	// The shader's array has a fixed unit, which needs GLSL 4.20.
	if (ShaderProgram::glslVersion < 420)
		return 0;
	ExtraMaps* own = OwnExtras(*this, material);
	if (own == nullptr || own->splatPath.empty())
		return 0;

	ExtraMaps& x = *own;
	const Scene3D::Season season = material.seasonal ? Scene3D::Get().GetSeason() : Scene3D::Season::Summer;
	// Made once per season; a set that will not load is remembered as 0 for
	// that season rather than tried again every frame.
	if (x.splatSeason != (int)season)
	{
		DropSplatArray(x);
		const char* suffix = (season == Scene3D::Season::Spring) ? "_spring"
			: (season == Scene3D::Season::Autumn) ? "_autumn"
			: (season == Scene3D::Season::Winter) ? "_winter" : "";
		x.splatTexture = BuildSplatArray(x.splatPath, suffix);
		x.splatSeason = (int)season;
		StartupStep("ground layers (materials.txt `splat`)");
	}
	return x.splatTexture;
}

float MaterialLibrary::Wind(const SceneMaterial& material, float* height) const
{
	if (height != nullptr)
		*height = 0.0f;
	const ExtraMaps* x = OwnExtras(*this, material);
	if (x == nullptr)
		return 0.0f;
	if (height != nullptr)
		*height = x->windHeight;
	return x->wind;
}

void MaterialLibrary::SetWind(SceneMaterial& material, float sway, float height)
{
	ExtrasChanged();
	if (sway == 0.0f && extraMaps.find(material.name) == extraMaps.end())
		return;
	ExtraMaps& x = extraMaps[material.name];
	x.wind = sway;
	x.windHeight = std::max(0.0f, height);
}

float MaterialLibrary::WindFlutter(const SceneMaterial& material) const
{
	const ExtraMaps* x = OwnExtras(*this, material);
	return x != nullptr ? x->windFlutter : 1.0f;
}

void MaterialLibrary::SetWindFlutter(SceneMaterial& material, float flutter)
{
	ExtrasChanged();
	if (flutter == 1.0f && extraMaps.find(material.name) == extraMaps.end())
		return;
	extraMaps[material.name].windFlutter = std::max(0.0f, flutter);
}

float MaterialLibrary::Translucency(const SceneMaterial& material) const
{
	const ExtraMaps* x = OwnExtras(*this, material);
	return x != nullptr ? x->translucency : 0.0f;
}

void MaterialLibrary::SetTranslucency(SceneMaterial& material, float strength)
{
	ExtrasChanged();
	if (strength == 0.0f && extraMaps.find(material.name) == extraMaps.end())
		return;
	extraMaps[material.name].translucency = std::max(0.0f, strength);
}

float MaterialLibrary::Cutout(const SceneMaterial& material) const
{
	const ExtraMaps* x = OwnExtras(*this, material);
	return x != nullptr ? x->cutout : 0.0f;
}

void MaterialLibrary::SetCutout(SceneMaterial& material, float threshold)
{
	ExtrasChanged();
	if (threshold == 0.0f && extraMaps.find(material.name) == extraMaps.end())
		return;
	extraMaps[material.name].cutout = std::max(0.0f, std::min(threshold, 1.0f));
}

bool MaterialLibrary::FadesNearLine(const SceneMaterial& material) const
{
	const ExtraMaps* x = OwnExtras(*this, material);
	return x != nullptr && x->fade;
}

void MaterialLibrary::SetFadesNearLine(SceneMaterial& material, bool fades)
{
	ExtrasChanged();
	if (!fades && extraMaps.find(material.name) == extraMaps.end())
		return;
	extraMaps[material.name].fade = fades;
}

bool MaterialLibrary::CastsShadow(const SceneMaterial& material) const
{
	const ExtraMaps* x = OwnExtras(*this, material);
	return x == nullptr || !x->noShadow;
}

void MaterialLibrary::SetCastsShadow(SceneMaterial& material, bool casts)
{
	ExtrasChanged();
	if (casts && extraMaps.find(material.name) == extraMaps.end())
		return;
	extraMaps[material.name].noShadow = !casts;
}

int MaterialLibrary::ShadowChoice(const SceneMaterial& material) const
{
	const ExtraMaps* x = OwnExtras(*this, material);
	if (x == nullptr)
		return 0;
	return x->noShadow ? -1 : x->forceShadow ? 1 : 0;
}

void MaterialLibrary::SetShadowChoice(SceneMaterial& material, int choice)
{
	ExtrasChanged();
	if (choice == 0 && extraMaps.find(material.name) == extraMaps.end())
		return;
	ExtraMaps& x = extraMaps[material.name];
	x.noShadow = choice < 0;
	x.forceShadow = choice > 0;
}

SceneMaterial* MaterialLibrary::Add(Game& game, const SceneMaterial& material)
{
	ExtrasChanged();   // (the list may move, or a caller change it)
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
		if (SplatLayersPath(*m) != s.maps.splatPath)
			SetSplatLayers(game, *m, s.maps.splatPath);
		SetWind(*m, s.maps.wind, s.maps.windHeight);
		SetWindFlutter(*m, s.maps.windFlutter);
		SetTranslucency(*m, s.maps.translucency);
		SetCutout(*m, s.maps.cutout);
		SetFadesNearLine(*m, s.maps.fade);
		SetShadowChoice(*m, s.maps.noShadow ? -1 : s.maps.forceShadow ? 1 : 0);
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
	ExtrasChanged();
	materials.clear();
	byName.clear();
	discarded.clear();
	for (auto& entry : extraMaps)
		DropSplatArray(entry.second);
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
		if (x.Any())
		{
			ExtraMaps& own = extraMaps[m.name];
			own.emissivePath = x.emissivePath;
			own.roughnessPath = x.roughnessPath;
			own.splatPath = x.splatPath;   // the layers load on first use (SplatLayersTexture)
			own.wind = x.wind;
			own.windHeight = x.windHeight;
			own.windFlutter = x.windFlutter;
			own.translucency = x.translucency;
			own.cutout = x.cutout;
			own.fade = x.fade;
			own.noShadow = x.noShadow;
			own.forceShadow = x.forceShadow;
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

unsigned int Scene3DInternal::MaterialLibraryVersion()
{
	return libraryVersion;
}
