// Scene3D lighting and materials: the Scene and Material uniform blocks.
// Split out of Scene3D.cpp (render-pass refactor, Phase 1 step 6).

#include "Scene3D.h"
#include "render/RenderDevice.h"
#include "Game.h"
#include "Renderer.h"
#include "Camera.h"
#include "Sprite.h"
#include "SpriteManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include "Skybox.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <map>
#include <tuple>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstddef>
#include "UniformBlocks.h"
#include "UniformBufferCache.h"
#include "RenderState.h"
#include "TransientBuffer.h"

#include "Scene3DInternal.h"

using Scene3DInternal::ProgramHasBlock;

namespace
{
	// std140 mirror of the GLSL "Scene" block (shaders/scene.glsl). vec3 + a
	// trailing scalar share one 16-byte slot; every array element (vec3, float
	// or int) takes a full 16 bytes, hence the vec4 / ivec4 arrays here (only
	// .xyz / .x are read).
	struct SceneBlockData
	{
		glm::vec3 ambientColor;      float lightningFlash;
		glm::vec3 dirLightDir;       float dirLightDiffuse;
		glm::vec3 dirLightColor;     float shadowStrength;
		glm::vec3 lightningColor;    float uTime;
		glm::vec3 viewPos;           int toon;
		glm::mat4 lightSpaceMatrix;
		int shadowsOn;
		int pointCount;
		int spotCount;
		int pointShadowCount;
		glm::vec4 pointPos[8];
		glm::vec4 pointColor[8];
		glm::vec4 pointRange[8];
		glm::vec4 pointIntensity[8];
		glm::vec4 spotPos[4];
		glm::vec4 spotDir[4];
		glm::vec4 spotColor[4];
		glm::vec4 spotRange[4];
		glm::vec4 spotIntensity[4];
		glm::vec4 spotCosInner[4];
		glm::vec4 spotCosOuter[4];
		glm::vec4 pointShadowPositions[8];
		glm::vec4 pointShadowFars[8];
		glm::ivec4 pointShadowLightIdx[8];
	};
	static_assert(sizeof(SceneBlockData) == 1504, "SceneBlockData must match shaders/scene.glsl");
	static_assert(offsetof(SceneBlockData, lightSpaceMatrix) == 80, "std140: mat4 starts on a 16-byte boundary");
	static_assert(offsetof(SceneBlockData, pointPos) == 160, "std140 offset of pointPos");

	const UniformBlockMember kSceneMembers[] = {
		{ "ambientColor", offsetof(SceneBlockData, ambientColor), false },
		{ "lightningFlash", offsetof(SceneBlockData, lightningFlash), false },
		{ "dirLightDir", offsetof(SceneBlockData, dirLightDir), false },
		{ "dirLightDiffuse", offsetof(SceneBlockData, dirLightDiffuse), false },
		{ "dirLightColor", offsetof(SceneBlockData, dirLightColor), false },
		{ "shadowStrength", offsetof(SceneBlockData, shadowStrength), false },
		{ "lightningColor", offsetof(SceneBlockData, lightningColor), false },
		{ "uTime", offsetof(SceneBlockData, uTime), false },
		{ "viewPos", offsetof(SceneBlockData, viewPos), false },
		{ "toon", offsetof(SceneBlockData, toon), false },
		{ "lightSpaceMatrix", offsetof(SceneBlockData, lightSpaceMatrix), false },
		{ "shadowsOn", offsetof(SceneBlockData, shadowsOn), false },
		{ "pointCount", offsetof(SceneBlockData, pointCount), false },
		{ "spotCount", offsetof(SceneBlockData, spotCount), false },
		{ "pointShadowCount", offsetof(SceneBlockData, pointShadowCount), false },
		{ "pointPos[0]", offsetof(SceneBlockData, pointPos), true },
		{ "pointColor[0]", offsetof(SceneBlockData, pointColor), true },
		{ "pointRange[0]", offsetof(SceneBlockData, pointRange), true },
		{ "pointIntensity[0]", offsetof(SceneBlockData, pointIntensity), true },
		{ "spotPos[0]", offsetof(SceneBlockData, spotPos), true },
		{ "spotDir[0]", offsetof(SceneBlockData, spotDir), true },
		{ "spotColor[0]", offsetof(SceneBlockData, spotColor), true },
		{ "spotRange[0]", offsetof(SceneBlockData, spotRange), true },
		{ "spotIntensity[0]", offsetof(SceneBlockData, spotIntensity), true },
		{ "spotCosInner[0]", offsetof(SceneBlockData, spotCosInner), true },
		{ "spotCosOuter[0]", offsetof(SceneBlockData, spotCosOuter), true },
		{ "pointShadowPositions[0]", offsetof(SceneBlockData, pointShadowPositions), true },
		{ "pointShadowFars[0]", offsetof(SceneBlockData, pointShadowFars), true },
		{ "pointShadowLightIdx[0]", offsetof(SceneBlockData, pointShadowLightIdx), true },
	};

	// std140 mirror of the GLSL "Material" block (shaders/material.glsl).
	struct MaterialBlockData
	{
		glm::vec3 matTint;      float matFresnel;
		glm::vec3 matEmissive;  float matNormalStrength;
		glm::vec2 matUVTile;
		float matSpecular;
		float matShininess;
		float matMetallic;
		float matRoughness;
		float matOpacity;
		int matHasNormal;
		int matNormalMode;
		int matLighting;
		float uWaterAmp;
		float uWaterWaveScale;
		float uWaterShoreFade;
		float uWaterChoppy;
		float pad[2];           // std140 rounds the block up to 16 bytes
	};
	static_assert(sizeof(MaterialBlockData) == 96, "MaterialBlockData must match shaders/material.glsl");

	const UniformBlockMember kMaterialMembers[] = {
		{ "matTint", offsetof(MaterialBlockData, matTint), false },
		{ "matFresnel", offsetof(MaterialBlockData, matFresnel), false },
		{ "matEmissive", offsetof(MaterialBlockData, matEmissive), false },
		{ "matNormalStrength", offsetof(MaterialBlockData, matNormalStrength), false },
		{ "matUVTile", offsetof(MaterialBlockData, matUVTile), false },
		{ "matSpecular", offsetof(MaterialBlockData, matSpecular), false },
		{ "matShininess", offsetof(MaterialBlockData, matShininess), false },
		{ "matMetallic", offsetof(MaterialBlockData, matMetallic), false },
		{ "matRoughness", offsetof(MaterialBlockData, matRoughness), false },
		{ "matOpacity", offsetof(MaterialBlockData, matOpacity), false },
		{ "matHasNormal", offsetof(MaterialBlockData, matHasNormal), false },
		{ "matNormalMode", offsetof(MaterialBlockData, matNormalMode), false },
		{ "matLighting", offsetof(MaterialBlockData, matLighting), false },
		{ "uWaterAmp", offsetof(MaterialBlockData, uWaterAmp), false },
		{ "uWaterWaveScale", offsetof(MaterialBlockData, uWaterWaveScale), false },
		{ "uWaterShoreFade", offsetof(MaterialBlockData, uWaterShoreFade), false },
		{ "uWaterChoppy", offsetof(MaterialBlockData, uWaterChoppy), false },
	};

	// Lighting rarely changes within a frame (a lightning flash does), so two
	// buffers suffice. A scene has a few dozen materials plus per-lake water
	// variants; 64 keeps them all resident.
	UniformBufferCache sceneBlocks(UniformBlock::Scene, sizeof(SceneBlockData), 2);
	UniformBufferCache materialBlocks(UniformBlock::Material, sizeof(MaterialBlockData), 64);

}

void Scene3D::ApplyLighting(unsigned int shaderID, const Renderer& renderer) const
{
	const int MAX_POINTS = 8;
	const int MAX_SPOTS = 4;
	const unsigned int id = shaderID;
	RenderDevice& device = Device();

	// Pack everything once into the std140 mirror of the Scene block. Programs
	// with the block get it uploaded (only when it changed); older programs get
	// the same values as loose uniforms below.
	SceneBlockData d = {};
	d.ambientColor = ambientColor;
	d.dirLightDir = dirLight.dir;
	d.dirLightColor = dirLight.color;
	d.dirLightDiffuse = dirLight.diffuse;
	// Storm lightning: a scene-wide flash of sky light (0 unless a strike is active).
	d.lightningFlash = flashIntensity;
	d.lightningColor = glm::vec3(0.80f, 0.85f, 1.0f);
	d.shadowStrength = shadowStrength;
	d.viewPos = renderer.camera.position;
	d.toon = celShading ? 1 : 0;
	// Seconds since start, for animated materials (water ripples).
	d.uTime = renderer.now * 0.001f;

	// Sun shadow map (bound to unit 3; unit 0 = albedo, 1 = normal map).
	if (shadowActive && shadowsEnabled && shadowDepthTex != 0)
	{
		device.BindTexture(3, TextureHandle(shadowDepthTex));
		Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "shadowMap")), (int)(3));
		d.lightSpaceMatrix = lightSpaceMatrix;
		d.shadowsOn = 1;
	}

	// Point lights: pack the ENABLED ones contiguously (off lights are skipped,
	// shrinking the count the shader loops over).
	int packedForSlot[kMaxPointShadows];   // cube-shadow caster slot -> packed index
	for (int s = 0; s < kMaxPointShadows; s++) packedForSlot[s] = -1;
	int pc = 0;
	for (const ScenePointLight& p : pointLights)
	{
		if (!p.on || p.guardHidden || pc >= MAX_POINTS) continue;
		d.pointPos[pc] = glm::vec4(p.pos, 0.0f);
		d.pointColor[pc] = glm::vec4(p.color, 0.0f);
		d.pointRange[pc].x = p.range;
		d.pointIntensity[pc].x = p.intensity;
		if (pointShadowActive)
			for (int s = 0; s < pointShadowCount; s++)
				if (p.pos == pointShadowPositions[s]) packedForSlot[s] = pc;
		pc++;
	}
	d.pointCount = pc;

	// Point-light (cube) shadows: textures to units 4, 5, ...; position, far and
	// the packed light index each caster shadows go in the block.
	int casters = 0;
	if (pointShadowActive && pointShadowsEnabled && pointShadowCount > 0)
	{
		const bool useArray = device.SupportsCubeMapArrays();
		if (useArray)
		{
			// GL4 cube-map array: the shader samples layer == caster slot, so
			// slots are used directly (no compaction) to keep layer/index aligned.
			for (int s = 0; s < pointShadowCount; s++)
			{
				d.pointShadowPositions[s] = glm::vec4(pointShadowPositions[s], 0.0f);
				d.pointShadowFars[s].x = pointShadowFars[s];
				d.pointShadowLightIdx[s].x = packedForSlot[s];
			}
			casters = pointShadowCount;
			device.BindTexture(4, TextureHandle(pointShadowArrayTex), TextureType::CubeArray);
			Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "pointShadowArray")), (int)(4));
		}
		else
		{
			// Fallback: separate cubes on units 4, 5, ... (compacted).
			int units[kMaxPointShadowsFallback];
			for (int s = 0; s < pointShadowCount && casters < kMaxPointShadowsFallback; s++)
			{
				if (pointShadowCubes[s] == 0 || packedForSlot[s] < 0) continue;
				device.BindTexture(4 + casters, TextureHandle(pointShadowCubes[s]), TextureType::Cube);
				units[casters] = 4 + casters;
				d.pointShadowPositions[casters] = glm::vec4(pointShadowPositions[s], 0.0f);
				d.pointShadowFars[casters].x = pointShadowFars[s];
				d.pointShadowLightIdx[casters].x = packedForSlot[s];
				casters++;
			}
			if (casters > 0)
				device.SetUniformArray(device.UniformLocation(ProgramHandle(id), "pointShadowMaps"), units, casters);
		}
	}
	d.pointShadowCount = casters;

	// Spot lights (enabled only), plus the runtime focus/debate spotlight.
	int sc = 0;
	auto packSpot = [&](const SceneSpotLight& s)
	{
		if (!s.on || sc >= MAX_SPOTS) return;
		d.spotPos[sc] = glm::vec4(s.pos, 0.0f);
		d.spotDir[sc] = glm::vec4(glm::normalize(s.dir), 0.0f);
		d.spotColor[sc] = glm::vec4(s.color, 0.0f);
		d.spotRange[sc].x = s.range;
		d.spotIntensity[sc].x = s.intensity;
		d.spotCosInner[sc].x = cosf(glm::radians(s.innerDeg));
		d.spotCosOuter[sc].x = cosf(glm::radians(s.outerDeg));
		sc++;
	};
	for (const SceneSpotLight& s : spotLights) packSpot(s);
	if (focusSpotOn) packSpot(focusSpot);
	d.spotCount = sc;

	if (ProgramHasBlock(id, "Scene"))
	{
		CheckUniformBlockLayout(id, "Scene", kSceneMembers,
			sizeof(kSceneMembers) / sizeof(kSceneMembers[0]), sizeof(SceneBlockData));
		sceneBlocks.Bind(&d);
		return;
	}

	// Legacy: a program that predates the Scene block (an old copy in a game's
	// data/shaders) reads the same values as loose uniforms. Arrays are
	// repacked from the block's 16-byte stride to tight vec3/float arrays.
	auto loc = [id](const char* name) { return Device().UniformLocation(ProgramHandle(id), name); };
	Device().SetUniform((int)(loc("ambientColor")), d.ambientColor);
	Device().SetUniform((int)(loc("dirLightDir")), d.dirLightDir);
	Device().SetUniform((int)(loc("dirLightColor")), d.dirLightColor);
	Device().SetUniform((int)(loc("dirLightDiffuse")), (float)(d.dirLightDiffuse));
	Device().SetUniform((int)(loc("lightningFlash")), (float)(d.lightningFlash));
	Device().SetUniform((int)(loc("lightningColor")), d.lightningColor);
	Device().SetUniform((int)(loc("viewPos")), d.viewPos);
	Device().SetUniform((int)(loc("toon")), (int)(d.toon));
	Device().SetUniform((int)(loc("uTime")), (float)(d.uTime));

	Device().SetUniform((int)(loc("shadowsOn")), (int)(d.shadowsOn));
	if (d.shadowsOn)
		Device().SetUniform((int)(loc("lightSpaceMatrix")), d.lightSpaceMatrix);
	if (d.shadowsOn || casters > 0)
		Device().SetUniform((int)(loc("shadowStrength")), (float)(d.shadowStrength));

	glm::vec3 v3[8];
	float f1[8];
	int i1[8];
	auto setVec3s = [&](const char* name, const glm::vec4* src, int n)
	{
		for (int k = 0; k < n; k++) v3[k] = glm::vec3(src[k]);
		device.SetUniformArray(loc(name), v3, n);
	};
	auto setFloats = [&](const char* name, const glm::vec4* src, int n)
	{
		for (int k = 0; k < n; k++) f1[k] = src[k].x;
		device.SetUniformArray(loc(name), f1, n);
	};

	Device().SetUniform((int)(loc("pointCount")), (int)(pc));
	if (pc > 0)
	{
		setVec3s("pointPos", d.pointPos, pc);
		setVec3s("pointColor", d.pointColor, pc);
		setFloats("pointRange", d.pointRange, pc);
		setFloats("pointIntensity", d.pointIntensity, pc);
	}

	Device().SetUniform((int)(loc("pointShadowCount")), (int)(casters));
	if (casters > 0)
	{
		setVec3s("pointShadowPositions", d.pointShadowPositions, casters);
		setFloats("pointShadowFars", d.pointShadowFars, casters);
		for (int k = 0; k < casters; k++) i1[k] = d.pointShadowLightIdx[k].x;
		device.SetUniformArray(loc("pointShadowLightIdx"), i1, casters);
	}

	Device().SetUniform((int)(loc("spotCount")), (int)(sc));
	if (sc > 0)
	{
		setVec3s("spotPos", d.spotPos, sc);
		setVec3s("spotDir", d.spotDir, sc);
		setVec3s("spotColor", d.spotColor, sc);
		setFloats("spotRange", d.spotRange, sc);
		setFloats("spotIntensity", d.spotIntensity, sc);
		setFloats("spotCosInner", d.spotCosInner, sc);
		setFloats("spotCosOuter", d.spotCosOuter, sc);
	}
}

void Scene3D::ApplyMaterial(unsigned int shaderID, const SceneMaterial& mat, const WaterSurface* water) const
{
	const unsigned int id = shaderID;

	// Normal map on unit 1 (if any); the device leaves unit 0 active.
	if (mat.normalMap != nullptr)
	{
		Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "normalMap")), (int)(1));
		mat.normalMap->UseTexture(1);
	}

	MaterialBlockData m = {};
	m.matTint = mat.tint;
	m.matEmissive = mat.emissive;
	m.matFresnel = mat.fresnel;
	m.matUVTile = mat.uvTile;
	m.matNormalStrength = mat.normalStrength;
	m.matNormalMode = (int)mat.normalMode;
	m.matLighting = (int)mat.lighting;
	m.matSpecular = mat.specular;
	m.matShininess = mat.shininess;
	m.matMetallic = mat.metallic;
	m.matRoughness = mat.roughness;
	m.matOpacity = mat.opacity;
	m.matHasNormal = (mat.normalMap != nullptr) ? 1 : 0;
	// The shader only reads these for water; defaults match the old uniform
	// initializers so non-water materials hash identically.
	m.uWaterAmp = 9.0f;
	m.uWaterWaveScale = 1.0f;
	m.uWaterShoreFade = 0.35f;
	m.uWaterChoppy = 1.0f;
	const bool isWater = (mat.lighting == LightingModel::Water && water != nullptr);
	if (isWater)
	{
		m.uWaterAmp = water->amplitude;
		m.uWaterWaveScale = water->waveScale;
		m.uWaterShoreFade = water->shoreFade;
		m.uWaterChoppy = water->choppy;
		m.matSpecular = water->specular;
		m.matShininess = water->shininess;
		m.matOpacity = water->opacity;
	}

	if (ProgramHasBlock(id, "Material"))
	{
		CheckUniformBlockLayout(id, "Material", kMaterialMembers,
			sizeof(kMaterialMembers) / sizeof(kMaterialMembers[0]), sizeof(MaterialBlockData));
		materialBlocks.Bind(&m);
		return;
	}

	// Legacy: loose uniforms for programs that predate the Material block.
	auto loc = [id](const char* name) { return Device().UniformLocation(ProgramHandle(id), name); };
	Device().SetUniform((int)(loc("matTint")), m.matTint);
	Device().SetUniform((int)(loc("matEmissive")), m.matEmissive);
	Device().SetUniform((int)(loc("matFresnel")), (float)(m.matFresnel));
	Device().SetUniform((int)(loc("matUVTile")), m.matUVTile);
	Device().SetUniform((int)(loc("matNormalStrength")), (float)(m.matNormalStrength));
	Device().SetUniform((int)(loc("matNormalMode")), (int)(m.matNormalMode));
	Device().SetUniform((int)(loc("matLighting")), (int)(m.matLighting));
	Device().SetUniform((int)(loc("matSpecular")), (float)(m.matSpecular));
	Device().SetUniform((int)(loc("matShininess")), (float)(m.matShininess));
	Device().SetUniform((int)(loc("matMetallic")), (float)(m.matMetallic));
	Device().SetUniform((int)(loc("matRoughness")), (float)(m.matRoughness));
	Device().SetUniform((int)(loc("matOpacity")), (float)(m.matOpacity));
	Device().SetUniform((int)(loc("matHasNormal")), (int)(m.matHasNormal));
	if (isWater)
	{
		Device().SetUniform((int)(loc("uWaterAmp")), (float)(m.uWaterAmp));
		Device().SetUniform((int)(loc("uWaterWaveScale")), (float)(m.uWaterWaveScale));
		Device().SetUniform((int)(loc("uWaterShoreFade")), (float)(m.uWaterShoreFade));
		Device().SetUniform((int)(loc("uWaterChoppy")), (float)(m.uWaterChoppy));
	}
}
