#ifndef DISTANCE_FOG_H
#define DISTANCE_FOG_H
#pragma once

// Engine-internal (not exported). Distance fog: the 3D world fades to one
// colour with its distance from the camera - none nearer than `near`, all of
// it from `far` on, linearly in between. Off unless the project, a scene, a
// script or the game turns it on.
//
// Unlike the volumetric fog (VolumetricFog.h) it works in both colour modes
// and costs one mix per pixel. The engine's lit shaders apply it
// (shaders/distance_fog.glsl: every exit of scene3d.frag, and
// billboard3d.frag) from the Scene block, which Scene3D::ApplyLighting fills.
// Weather and fountain particles fade out into it, and the toon outline takes
// its colour (the Outline block). The panorama skybox stays unfogged.
//
//   renderer.dat  `distanceFog 0|1`, `distanceFogColor r g b`,
//                 `distanceFogNear`, `distanceFogFar` (the project's default)
//   .scene        `distfog <r g b> <near> <far>`, or `distfog off`
//   script        scene3d distfog <r g b> <near> <far> [seconds] | off [seconds]
//   code          Scene3D::SetDistanceFog
//   testing       KINJO_DISTFOG="r g b near far", or KINJO_DISTFOG=0 (off)
//
// Colours are authored (sRGB, 0..1). The Scene block gets them through
// SceneColor, so in a linear-workflow project the fog mixes in linear light,
// before exposure and the tonemapper.

#include <glm/vec3.hpp>

struct DistanceFogSettings
{
	bool on = false;
	glm::vec3 color = glm::vec3(0.6f, 0.65f, 0.7f);   // authored sRGB, 0..1
	float nearDistance = 1000.0f;   // world units from the camera: no fog nearer
	float farDistance = 5000.0f;    // all fog from here on
};

// What the shaders get this frame: the settings in force with any fade
// applied. amount 0 = no fog; between 0 and 1 while it fades in or out.
struct DistanceFogFrame
{
	glm::vec3 color = glm::vec3(0.0f);   // authored sRGB
	float nearDistance = 0.0f;
	float farDistance = 0.0f;
	float amount = 0.0f;
};

// renderer.dat and KINJO_DISTFOG: at startup, and again when the editor's
// PROJECT page changes renderer.dat.
void LoadDistanceFogSettings();

// The current scene's fog (`set` false = the project's), fading to it over
// `seconds` from what shows now. Reset (set false, no fade) on every scene
// load and unload.
void SetSceneDistanceFog(bool set, const DistanceFogSettings& fog, float seconds);
bool GetSceneDistanceFog(DistanceFogSettings& fog);   // false = the scene sets none (for saving)
// The settings in force, before any fade: KINJO_DISTFOG's, else the scene's,
// else the project's. For the editor.
DistanceFogSettings DistanceFogInForce();
DistanceFogFrame CurrentDistanceFog();

#endif
