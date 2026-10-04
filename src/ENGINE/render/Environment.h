#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
#pragma once

// Engine-internal (not exported). Image-based lighting - Phase 1.5 item 4
// (docs/RENDERING_BACKEND_PLAN.md). Linear workflow only.
//
// The scene's sky panorama (with its cross-fade and tint, so it follows time
// of day) is captured into environment maps whenever it changes:
//   envBase      512x256 linear HDR panorama, mipmapped     (env_capture.frag)
//   irradiance   64x32 cosine-convolved panorama            (env_irradiance.frag)
//   specular     256x128 panorama, 6 mips = roughness 0..1  (env_specular.frag)
//   BRDF LUT     128x128 split-sum table, computed once     (brdf_lut.frag)
// Lit shaders read them through shaders/environment.glsl: ambient becomes the
// sky's light from the surface's direction (replacing the flat `ambient`
// colour), and surfaces reflect the sky by roughness and Fresnel.
//
//   renderer.dat  `ibl <diffuse> [specular]`   default 1 1; `ibl 0` = off
//   .scene        `ibl <diffuse> [specular]`   that scene's own values
//   KINJO_IBL=0 / =1 overrides for one run (testing).
// Scenes without a sky keep their flat ambient.

#include <glm/vec3.hpp>

class Texture;

struct SkySource
{
	Texture* sky = nullptr;
	Texture* next = nullptr;          // cross-fade target (time-of-day), or null
	float blend = 0.0f;               // toward `next`, 0..1
	glm::vec3 tint = glm::vec3(1.0f); // the sky's tint as authored (sRGB, 0..1)
};

void LoadEnvironmentSettings();       // renderer.dat / KINJO_IBL (after LoadColorSettings)

// Once per frame before the world: what the sky looks like now (hasSky false
// for a scene without one). Decides whether IBL is on and the maps are stale.
void SetEnvironmentSource(bool hasSky, const SkySource& source);
bool EnvironmentDirty();              // run UpdateEnvironment (in its own pass)
void UpdateEnvironment(int screenWidth, int screenHeight);   // restores the viewport to this
bool EnvironmentActive();             // IBL lights this frame

// Per lit draw (Scene3D::ApplyLighting): the Environment block, and the maps
// on texture units 8-10 when active. Always safe to call.
void BindEnvironment(unsigned int program);

// The current scene's IBL strengths: < 0 = the project default.
void SetSceneIBL(float diffuse, float specular);
void GetSceneIBL(float& diffuse, float& specular);
void GetProjectIBL(float& diffuse, float& specular);   // renderer.dat `ibl` / `iblSpecular`

void ReleaseEnvironment();

#endif
