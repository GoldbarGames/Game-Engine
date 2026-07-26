#ifndef SCENEMATERIAL_H
#define SCENEMATERIAL_H
#pragma once

// Material system for 3D scene models (Scene3D). A material adds surface
// qualities on top of a model's albedo texture: specular highlights, a
// tangent-space normal map (fakes depth on a flat texture), emissive glow, a
// fresnel rim, an albedo tint and UV tiling. Each material picks its lighting
// model (Blinn-Phong or PBR metallic/roughness) and how normal mapping derives
// its tangent basis (screen-space derivatives or real vertex tangents).
//
// Materials are named and loaded from data/materials.txt (see MaterialLibrary),
// then referenced per model by the .scene "mat <name>" token. Engine feature -
// available to any 3D game.

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include "leak_check.h"

class Game;
class Texture;

// Phong / PBR are surface lighting models; Water is a special animated mode
// (procedural ripples + fresnel sky reflection + sun glints) for lake/sea planes.
enum class LightingModel { Phong = 0, PBR = 1, Water = 2 };
enum class NormalMode { ScreenSpace = 0, Vertex = 1 };

class KINJO_API SceneMaterial
{
public:
	std::string name = "default";

	// Common (both lighting models)
	glm::vec3 tint = glm::vec3(1.0f);        // multiplies the albedo texture
	glm::vec3 emissive = glm::vec3(0.0f);    // self-illumination added at the end
	float fresnel = 0.0f;                    // view-angle rim (0 = off; ice/glass)
	glm::vec2 uvTile = glm::vec2(1.0f);      // texture repeat

	// Normal map (tangent space). Empty path = use the geometric normal.
	std::string normalMapPath;
	Texture* normalMap = nullptr;            // resolved by MaterialLibrary::Load
	float normalStrength = 1.0f;
	NormalMode normalMode = NormalMode::ScreenSpace;

	// Blinn-Phong parameters
	float specular = 0.2f;                   // highlight strength
	float shininess = 24.0f;                 // highlight tightness

	// PBR parameters
	float metallic = 0.0f;
	float roughness = 0.5f;

	LightingModel lighting = LightingModel::Phong;

	// Transparency (Phase: transparent pass). 1 = opaque.
	float opacity = 1.0f;
	bool IsTransparent() const { return opacity < 0.999f; }

	// Cel-outline opt-out: set "outline off" on room-shell materials (floors,
	// walls, ceilings) where an inverted-hull silhouette is ill-defined and
	// looks wrong; props keep it. Only consulted when Scene3D::celShading is on.
	bool outline = true;
};

// Named material registry, loaded once from a data file. Singleton like
// Scene3D so the editor and renderer share it.
class KINJO_API MaterialLibrary
{
public:
	static MaterialLibrary& Get();

	// Load material definitions (resolving normal-map textures via the sprite
	// manager). Safe to call again to reload. Returns false if the file is
	// missing (the library still provides the built-in "default" material).
	bool Load(Game& game, const std::string& path = "data/materials.txt");

	const SceneMaterial* Find(const std::string& name) const;
	const SceneMaterial& Default() const { return defaultMat; }
	std::vector<std::string> Names() const;        // for the editor dropdown
	const std::vector<SceneMaterial>& All() const { return materials; }

private:
	// Untagged models fall back to defaultMat, which opts OUT of the cel
	// outline (an inverted-hull silhouette needs a clean convex-ish mesh; give
	// a prop an explicit material to outline it).
	MaterialLibrary() { defaultMat.outline = false; }
	std::vector<SceneMaterial> materials;
	std::map<std::string, int> byName;
	SceneMaterial defaultMat;   // plain matte fallback
};

#endif
