#ifndef CLUSTERED_LIGHTS_H
#define CLUSTERED_LIGHTS_H
#pragma once

// Engine-internal (not exported). Clustered forward lighting ("Forward+") -
// Phase 1.5 item 8 (docs/RENDERING_BACKEND_PLAN.md). Any colour mode.
//
// The view frustum is cut into 16 x 9 screen tiles x 24 depth slices
// (exponential in depth). Each frame the CPU assigns every point and spot
// light to the clusters its sphere of influence overlaps, and uploads three
// things into ONE RGBA32UI texture: the light records, a table of each
// cluster's (first, count) into the light lists, and the lists themselves.
// A lit pixel finds its cluster from its screen position and view depth and
// shades only that cluster's lights (shaders/lights.glsl), so a scene can
// carry hundreds of lights while each pixel pays for the handful near it.
//
// CPU assignment rather than compute because WebGL2 has none; one texture
// rather than several because WebGL2 guarantees only 16 fragment samplers
// and the lit shaders already use 13 of them.
//
//   renderer.dat  `clusteredLights 1|0`   default 1; 0 = the Scene block's
//                                         first 8 point + 4 spot lights only
//   KINJO_CLUSTERS=0 / =1 overrides it for one run;
//   KINJO_CLUSTER_DEBUG=1 colours lit surfaces by how many lights reach them.

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

// One light as lit shaders receive it. Colours are already what the shader
// multiplies (SceneColor-converted); `shadow` is the light's point-shadow
// caster index in the Scene block, or -1.
struct ClusterLight
{
	glm::vec3 pos = glm::vec3(0.0f);
	float range = 0.0f;
	glm::vec3 color = glm::vec3(0.0f);
	float intensity = 0.0f;
	glm::vec3 dir = glm::vec3(0.0f, 1.0f, 0.0f);   // spot only (normalised)
	float cosOuter = 0.0f;
	float cosInner = 0.0f;
	bool spot = false;
	int shadow = -1;
};

void LoadClusteredLightSettings();
bool ClusteredLightsWanted();

// Once per frame, before the world: assign `lights` (points first, then spots
// - the order lit shaders sum them in) to the clusters of the camera that
// draws the world, and upload them. `width`/`height` = the world target.
void BuildLightClusters(const std::vector<ClusterLight>& lights, const glm::mat4& view,
	const glm::mat4& projection, float nearPlane, float farPlane, int width, int height);
// This frame has none (no 3D scene): lit shaders fall back to the Scene block.
void DisableLightClusters();

// Per lit draw (Scene3D::ApplyLighting): the Clusters block, and the light
// texture on unit 14. Always safe to call.
void BindLightClusters(unsigned int program);

// Colour lit surfaces by how many lights reach them (KINJO_CLUSTER_DEBUG; the
// 3D editor's LOOK panel toggles it at runtime).
void SetClusterDebugView(bool on);
bool ClusterDebugView();

// Split screen (render/RenderViews.h): which view's clusters are built and
// bound. 0 outside split screen.
void SetLightClusterView(int index);

void ReleaseClusteredLights();

#endif
