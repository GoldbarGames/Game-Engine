#ifndef MODEL_MATERIALS_H
#define MODEL_MATERIALS_H
#pragma once

// Engine-internal (not exported). Materials that come inside a model file:
// glTF 2.0 metallic-roughness (Phase 1.5 item 10). Model::LoadModel imports
// them; Scene3D draws each mesh with its own. They are kept per GPU mesh here,
// so Model, Mesh and Scene3DModel keep their exported layouts.
//
// Texture units, as scene3d.frag declares them: base colour 0, normal map 1,
// metallic-roughness 2 (G = roughness, B = metallic, and R = occlusion when
// the file packs it there), occlusion 5 and emissive 6. Units 5 and 6 belong
// to the point-shadow cubes on the 3.3/web fallback, so there those two maps
// are left out (their factors still apply). See ModelMaterialExtraMaps().

#include "SceneMaterial.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

class Mesh;
class Texture;

enum class AlphaMode : uint8_t { Opaque = 0, Mask = 1, Blend = 2 };

// What the file says (texture paths resolved; cached in the model's .kmesh).
struct ModelMaterialDesc
{
	std::string baseColorMap;           // "" = none
	std::string metallicRoughnessMap;
	std::string normalMap;
	std::string occlusionMap;
	std::string emissiveMap;
	glm::vec4 baseColor = glm::vec4(1.0f);   // linear RGB + alpha
	float metallic = 1.0f;
	float roughness = 1.0f;
	float normalScale = 1.0f;
	float occlusionStrength = 1.0f;
	glm::vec3 emissive = glm::vec3(0.0f);    // linear, emissive strength applied
	AlphaMode alphaMode = AlphaMode::Opaque;
	float alphaCutoff = 0.5f;
	bool doubleSided = false;
	bool unlit = false;                      // KHR_materials_unlit
};

// Ready to draw.
struct ModelMaterial
{
	SceneMaterial scene;                // what ApplyMaterial reads: PBR, tint, opacity, emissive, normal map
	Texture* baseColor = nullptr;       // null = white
	Texture* metallicRoughness = nullptr;
	Texture* occlusion = nullptr;       // null when absent, packed, or on the 3.3/web fallback
	bool occlusionPacked = false;       // occlusion is metallicRoughness's R channel
	Texture* emissive = nullptr;        // null on the fallback (scene.emissive holds the map's average there)
	AlphaMode alphaMode = AlphaMode::Opaque;
	float alphaCutoff = 0.5f;
	float occlusionStrength = 1.0f;
	bool doubleSided = false;
	bool unlit = false;
};

// Builds a drawable material, loading its textures (each file once per
// process: colour maps sRGB in a linear-workflow project, data maps linear).
const ModelMaterial* CreateModelMaterial(const ModelMaterialDesc& desc);

void SetMeshMaterial(const Mesh* mesh, const ModelMaterial* material);
// The material a mesh brought with it; null = draw with the model's scene material.
const ModelMaterial* MeshMaterial(const Mesh* mesh);
void ForgetMeshMaterial(const Mesh* mesh);
bool HasModelMaterials(const std::vector<Mesh*>& meshes);
// Whether any mesh is alpha-blended (drawn in the transparent pass).
bool HasBlendedModelMaterial(const std::vector<Mesh*>& meshes);

// A 1x1 white texture (made on first use): the base colour of a material
// without a base-colour map, and the stand-in for a model with no texture.
Texture* ModelWhiteTexture();
// What a mesh with this material binds on unit 0: its base-colour map, or white.
Texture* ModelBaseColor(const ModelMaterial& material);
// What the depth-only passes bind for alpha cut-outs: the base colour of a
// masked or blended material; white (never cut) for an opaque one, whose
// texture alpha means nothing in glTF.
Texture* ModelShadowAlpha(const ModelMaterial& material);

// Whether this context's scene3d.frag samples the occlusion and emissive maps
// (KINJO_GL4: units 5 and 6 are free there).
bool ModelMaterialExtraMaps();

#endif
