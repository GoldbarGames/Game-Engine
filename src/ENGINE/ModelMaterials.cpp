// Materials imported with a model - see ModelMaterials.h.

#include "ModelMaterials.h"
#include "Texture.h"
#include "Shader.h"
#include "globals.h"
#include "render/ColorPipeline.h"
#include "render/TextureFiles.h"
#include <cmath>
#include <iostream>
#include <map>
#include <unordered_map>

namespace
{
	// Never destroyed: meshes and textures can outlive static destruction.
	std::unordered_map<const Mesh*, const ModelMaterial*>& Meshes()
	{
		static auto* meshes = new std::unordered_map<const Mesh*, const ModelMaterial*>();
		return *meshes;
	}

	std::map<std::string, Texture*>& Textures()
	{
		static auto* textures = new std::map<std::string, Texture*>();
		return *textures;
	}

	// The inverse of SrgbToLinear: SceneColor (in ApplyMaterial) turns these
	// back into the file's linear values in a linear-workflow project, and a
	// gamma-space shader gets the encoded colour its sRGB textures hold.
	float LinearToSrgb(float v)
	{
		v = std::max(v, 0.0f);
		return (v <= 0.0031308f) ? v * 12.92f : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
	}

	glm::vec3 LinearToSrgb(const glm::vec3& c)
	{
		return glm::vec3(LinearToSrgb(c.r), LinearToSrgb(c.g), LinearToSrgb(c.b));
	}

	// Smooth (mipmapped, anisotropic) and repeating, like every Scene3D
	// texture. Colour maps are sRGB in a linear-workflow project.
	Texture* LoadMap(const std::string& path, bool color)
	{
		if (path.empty())
			return nullptr;
		const bool srgb = color && LinearWorkflow();
		const std::string key = path + (srgb ? "\n[srgb]" : "");
		auto it = Textures().find(key);
		if (it != Textures().end())
			return it->second;

		Texture* texture = new Texture(path);
		std::vector<unsigned char> bytes;
		const std::string file = PreferredTextureFile(path);   // a .ktx2 beside it wins
		bool ok = ReadFileBytes(file, bytes)
			&& texture->LoadFromFileData(bytes.data(), bytes.size(), Texture::Filter::Smooth, srgb);
		if (!ok && file != path)
		{
			bytes.clear();
			ok = ReadFileBytes(path, bytes)
				&& texture->LoadFromFileData(bytes.data(), bytes.size(), Texture::Filter::Smooth, srgb);
		}
		if (!ok)
		{
			std::cout << "Model texture " << path << " failed to load" << std::endl;
			delete_it(texture);
			texture = nullptr;
		}
		Textures()[key] = texture;
		return texture;
	}

	// The mean linear colour of an sRGB image file SDL_image decodes (PNG,
	// JPEG...). False for a KTX2 file (no BC decoder here) or a bad image.
	bool AverageColor(const std::string& path, glm::vec3& out)
	{
		if (IsKtx2Path(path))
			return false;
		SDL_Surface* loaded = IMG_Load(path.c_str());
		if (loaded == nullptr)
			return false;
		SDL_Surface* s = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(loaded);
		if (s == nullptr)
			return false;
		float decode[256];
		for (int i = 0; i < 256; i++)
			decode[i] = SrgbToLinear(glm::vec3(i / 255.0f)).r;
		glm::dvec3 sum(0.0);
		for (int y = 0; y < s->h; y++)
		{
			const unsigned char* row = (const unsigned char*)s->pixels + y * s->pitch;
			for (int x = 0; x < s->w; x++)
				sum += glm::dvec3(decode[row[x * 4]], decode[row[x * 4 + 1]], decode[row[x * 4 + 2]]);
		}
		const double n = std::max(1.0, (double)s->w * s->h);
		out = glm::vec3(sum / n);
		SDL_FreeSurface(s);
		return true;
	}
}

const ModelMaterial* CreateModelMaterial(const ModelMaterialDesc& desc)
{
	// Kept for the process, like the meshes it belongs to (Model.cpp's cache).
	ModelMaterial* m = new ModelMaterial();

	SceneMaterial& s = m->scene;
	s.name = "(model)";
	s.lighting = LightingModel::PBR;
	s.tint = LinearToSrgb(glm::vec3(desc.baseColor));
	s.emissive = LinearToSrgb(desc.emissive);
	s.metallic = desc.metallic;
	s.roughness = desc.roughness;
	s.normalStrength = desc.normalScale;
	s.normalMode = NormalMode::ScreenSpace;
	// The base colour's alpha: a blended surface's opacity. A masked one
	// compares texture alpha x factor with the cutoff, so the factor folds into
	// the cutoff instead (the surface itself stays opaque: opacity < 1 would
	// count as see-through, e.g. for motion vectors). Opaque ignores alpha.
	s.opacity = (desc.alphaMode == AlphaMode::Blend) ? desc.baseColor.a : 1.0f;
	m->alphaCutoff = desc.alphaCutoff;
	if (desc.alphaMode == AlphaMode::Mask)
		m->alphaCutoff = (desc.baseColor.a > 0.0f) ? desc.alphaCutoff / desc.baseColor.a : 2.0f;
	s.outline = true;

	m->baseColor = LoadMap(desc.baseColorMap, true);
	m->metallicRoughness = LoadMap(desc.metallicRoughnessMap, false);
	s.normalMap = LoadMap(desc.normalMap, false);
	s.normalMapPath = desc.normalMap;
	m->occlusionPacked = !desc.occlusionMap.empty() && desc.occlusionMap == desc.metallicRoughnessMap;
	if (ModelMaterialExtraMaps())
	{
		if (!m->occlusionPacked)
			m->occlusion = LoadMap(desc.occlusionMap, false);
		m->emissive = LoadMap(desc.emissiveMap, true);
	}
	else if (!desc.emissiveMap.empty())
	{
		// No unit for the map here (the 3.3/web fallback): glow with its
		// average colour rather than the bare factor (often white).
		glm::vec3 average(0.0f);
		if (!AverageColor(desc.emissiveMap, average))
			std::cout << "Model texture " << desc.emissiveMap
				<< ": emissive maps need GL 4 here; the surface won't glow" << std::endl;
		s.emissive = LinearToSrgb(desc.emissive * average);
	}

	m->alphaMode = desc.alphaMode;
	m->occlusionStrength = desc.occlusionStrength;
	m->doubleSided = desc.doubleSided;
	m->unlit = desc.unlit;
	return m;
}

void SetMeshMaterial(const Mesh* mesh, const ModelMaterial* material)
{
	if (material != nullptr)
		Meshes()[mesh] = material;
	else
		ForgetMeshMaterial(mesh);
}

const ModelMaterial* MeshMaterial(const Mesh* mesh)
{
	if (Meshes().empty())
		return nullptr;
	auto it = Meshes().find(mesh);
	return it == Meshes().end() ? nullptr : it->second;
}

void ForgetMeshMaterial(const Mesh* mesh)
{
	if (!Meshes().empty())
		Meshes().erase(mesh);
}

bool HasModelMaterials(const std::vector<Mesh*>& meshes)
{
	if (Meshes().empty())
		return false;
	for (const Mesh* mesh : meshes)
		if (MeshMaterial(mesh) != nullptr)
			return true;
	return false;
}

bool HasBlendedModelMaterial(const std::vector<Mesh*>& meshes)
{
	if (Meshes().empty())
		return false;
	for (const Mesh* mesh : meshes)
	{
		const ModelMaterial* m = MeshMaterial(mesh);
		if (m != nullptr && m->alphaMode == AlphaMode::Blend)
			return true;
	}
	return false;
}

Texture* ModelWhiteTexture()
{
	static Texture* white = nullptr;
	if (white == nullptr)
	{
		white = new Texture("(white)");
		SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
		if (s != nullptr)
		{
			SDL_FillRect(s, nullptr, SDL_MapRGBA(s->format, 255, 255, 255, 255));
			white->LoadTexture(s, false, Texture::Filter::Point, false);
			SDL_FreeSurface(s);
		}
	}
	return white;
}

Texture* ModelBaseColor(const ModelMaterial& material)
{
	return material.baseColor != nullptr ? material.baseColor : ModelWhiteTexture();
}

Texture* ModelShadowAlpha(const ModelMaterial& material)
{
	return material.alphaMode == AlphaMode::Opaque ? ModelWhiteTexture() : ModelBaseColor(material);
}

bool ModelMaterialExtraMaps()
{
#ifdef __EMSCRIPTEN__
	return false;
#else
	return ShaderProgram::glslVersion >= 400;   // the KINJO_GL4 define (Shader.cpp)
#endif
}
