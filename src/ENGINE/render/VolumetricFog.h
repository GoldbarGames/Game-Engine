#ifndef VOLUMETRIC_FOG_H
#define VOLUMETRIC_FOG_H
#pragma once

// Engine-internal (not exported). Volumetric fog and light shafts - Phase 1.5
// item 9 (docs/RENDERING_BACKEND_PLAN.md). Linear workflow only.
//
// Height fog that lights itself: a half-resolution ray march from the camera
// to each pixel's surface (shaders/fog_march.frag) gathers the sun through the
// cascaded shadows (god rays), every clustered point/spot light through its
// cube shadow (shafts from lamps), the sky's ambient light and storm lightning;
// a depth-aware upsample lays it over the world before temporal anti-aliasing,
// which smooths the march's per-frame noise (fog_composite.frag).
//
// Units are the world's: density is extinction per world unit (0.0005 ~ half
// the light lost over 1400 units), falloff thins it with height above `base`
// (world up is -Y), anisotropy g > 0 scatters forwards (shafts glow when you
// look towards the light).
//
//   renderer.dat  `fog <density>` (default 0 = none), `fogHeightFalloff`,
//                 `fogColor r g b`, `fogAnisotropy`, `fogNoise`, `fogDistance`,
//                 `volumetricFog 1|0` (master switch, default 1)
//   .scene        `fog <density> [falloff] [r g b] [anisotropy] [noise]`
//   Rain, snow and storms bring their own fog when the scene doesn't set one.
//   scene3d fog <density> [seconds]; KINJO_FOG="<density> [falloff]" (testing).

#include "RenderDevice.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct FogSettings
{
	float density = 0.0f;
	float heightFalloff = 0.004f;   // per world unit of height
	float baseHeight = 0.0f;        // world height (-y) where the falloff starts
	glm::vec3 color = glm::vec3(1.0f);   // scattering colour (authored sRGB)
	float anisotropy = 0.5f;
	float noise = 0.0f;
};

void LoadFogSettings();

// The current scene's fog (`set` false = the project's / weather default),
// its density fading over `seconds`.
void SetSceneFog(bool set, const FogSettings& fog, float seconds);
bool GetSceneFog(FogSettings& fog);           // false = the scene sets none (for saving)
// The settings in force, before any density fade: the scene's, else the
// project's (with the weather's density when the project sets none).
FogSettings FogInForce();
void SetSceneFogDensity(float density, float seconds);   // scene3d fog <density> [seconds]
// The weather's fog (rain/snow/storm), used when the scene sets none.
void SetWeatherFogDensity(float density);

bool FogActive();   // linear, allowed, density > 0 this frame

// The march program (for Scene3D to bind its lighting to), after making sure
// the targets exist for this size; 0 = can't run.
unsigned int BeginFogMarch(int width, int height);
// Draw the march (program in use, lighting bound), then lay it over the world.
void DrawFogMarch(const glm::mat4& invViewProj, TextureHandle depth, int width, int height, float timeSeconds);
void CompositeFog(TextureHandle depth, float nearPlane, float farPlane, int width, int height);

void ReleaseVolumetricFog();

#endif
