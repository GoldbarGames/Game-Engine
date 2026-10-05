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
//
// Two more maps live in the library rather than in this class (its layout is
// fixed by the DLL's ABI): `emissivemap <png>`, a glow map multiplying
// `emissive` (lit windows, signs; GL 4 only), and `roughnessmap <png>`, whose
// green channel multiplies a PBR material's roughness and blue its metallic
// (glTF's packing: a grey image scales both; puddles on a road). See
// MaterialLibrary::SetEmissiveMap / SetRoughnessMap.

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

	// Seasonal: a model using this material swaps its texture to a per-season
	// variant (base "_spring/_autumn/_winter" before the extension) when the scene
	// season changes; summer / a missing variant uses the base texture. Set on
	// grass/foliage materials via the "seasonal" material token.
	bool seasonal = false;

	// Deciduous: a model using this material sheds its leaves in winter - the
	// season swap replaces its mesh with a bare-branch variant ("<obj>_bare.obj")
	// instead of (or in addition to) the texture swap. Set on the deciduous
	// tree-canopy material via the "deciduous" token; evergreens leave it off.
	bool deciduous = false;
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

	// --- editing (the 3D editor's MATERIAL panel) ---------------------------
	// Edits apply in place, so every model using a material updates at once.
	SceneMaterial* FindMutable(const std::string& name);
	// A new material (or one made earlier this session and undone). The list
	// may move in memory: refresh held pointers (Scene3DModel::material) after.
	SceneMaterial* Add(Game& game, const SceneMaterial& material);
	// Point a material at a normal map, loading it ("" = none).
	void SetNormalMap(Game& game, SceneMaterial& material, const std::string& path);
	// The same for its glow map (`emissivemap`) and roughness map
	// (`roughnessmap`). They belong to the library's own material of that name.
	void SetEmissiveMap(Game& game, SceneMaterial& material, const std::string& path);
	void SetRoughnessMap(Game& game, SceneMaterial& material, const std::string& path);
	std::string EmissiveMapPath(const SceneMaterial& material) const;   // "" = none
	std::string RoughnessMapPath(const SceneMaterial& material) const;
	// The loaded maps (nullptr: none, or `material` isn't one of the library's).
	Texture* EmissiveMap(const SceneMaterial& material) const;
	Texture* RoughnessMap(const SceneMaterial& material) const;
	// Ground splatting (`splat` in a material file): up to three more textures
	// blended over the model's own, weighted per vertex by the mesh's tangent
	// slot - x, y and z for the three layers, the model's texture taking what
	// is left (shaders/scene3d.frag, MAT_SPLAT). For meshes built for it, such
	// as a game's terrain; not for imported models, whose tangent slot holds
	// tangents. `paths` lists the files, space-separated ("" = none). A
	// `seasonal` material's layers follow the season like its texture does.
	// GLSL 4.20+ only: elsewhere the model shows its own texture alone.
	void SetSplatLayers(Game& game, SceneMaterial& material, const std::string& paths);
	std::string SplatLayersPath(const SceneMaterial& material) const;   // "" = none
	// The layers as a texture array for the current season, made on first use
	// (0: none, or this GPU can't). For the renderer (Scene3D::ApplyMaterial).
	unsigned int SplatLayersTexture(const SceneMaterial& material) const;
	// The whole library as material-file text (the editor's undo snapshots),
	// and back: materials missing from the text are dropped from Names() and
	// saving, as if never made. Then refresh held pointers, as after Add.
	std::string Serialize() const;
	void ApplySerialized(Game& game, const std::string& text);
	// Write the materials that differ from the file Load read back into it,
	// changing only their differing lines (comments and layout stay); new
	// materials are appended. `message` says what happened. False on failure.
	bool SaveChanges(std::string& message) const;
	const std::string& LoadedPath() const;

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
