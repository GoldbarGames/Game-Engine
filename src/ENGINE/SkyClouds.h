#ifndef KINJO_SKY_CLOUDS_H
#define KINJO_SKY_CLOUDS_H
#pragma once

// CLOUDS THAT MOVE (SkyClouds.cpp, shaders/sky_clouds.*; docs/VISUAL_EFFECTS.md).
//
// Up to two layers of cloud over the sky's panorama, worked out per pixel each
// frame: each pixel's direction is followed up to a flat layer of cloud and
// noise is read where it lands, so a cloud overhead is big and one near the
// horizon small and crowded behind its neighbours - and the layer drifts with
// the wind, and slides overhead as the camera travels under it. A painted
// cloud is nailed to the sky; these are not.
//
//   SetSkyClouds(layers, n)      what the layers are (0..2; 0 = none, the default)
//   SetSkyCloudLight(...)        where the sun is, and the light their edges catch
//   SetSkyCloudWind(v)           how fast the air up there moves
//
// Two kinds of layer:
//   * a SHEET: cirrus combed out by the wind, an overcast deck, scud under it;
//   * HEAPED: fair-weather cumulus, a slab marched through per pixel, so seen
//     from below a cloud is its flat base and low down the heaps are seen from
//     the side, tops in the sun and bases in shade.
// They fade into the panorama with distance, so the panorama's own horizon
// haze shows through, and the sun's glare stays in front of them.
//
// Drawn by Skybox::Render over the panorama (both of its cross-faded skies),
// so the panorama should be painted without clouds of its own: a sky with
// moving clouds over painted ones looks like two skies. A game that cross-
// fades its skies sets layers blended to match (TrainRails' ApplySkyTextures).
// Units are the world's, up is -Y; colours are authored sRGB 0..1.

#include "leak_check.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct SkyCloudLayer
{
	bool heaped = false;          // cumulus (a marched slab) rather than a flat sheet
	float height = 1500.0f;       // the layer's base, world units above the camera
	float size = 940.0f;          // a cloud's size: world units across one cell of the noise
	float cover = 0.56f;          // noise below this is clear sky (0..1: higher, fewer clouds)
	float soft = 0.16f;           // ...feathered over this much
	float warp = 0.55f;           // how much the shapes are swirled
	float opacity = 1.0f;
	float depth = 0.6f;           // heaped: how tall the heaps stand, as a fraction of `height`
	glm::vec2 stretch = glm::vec2(1.0f);   // drawn out along a direction: cirrus, combed by the wind
	float angle = 0.0f;           // ...that direction, radians round from world +x
	float hazeDistance = 50000.0f;   // fades into the sky: 63% gone this far off, world units
	glm::vec3 lit = glm::vec3(1.0f, 0.99f, 0.96f);     // in the sun
	glm::vec3 shade = glm::vec3(0.60f, 0.66f, 0.76f);  // in its own shade
	float silver = 0.5f;          // the bright rim of a thin edge near the sun (or the moon)
	float thickDark = 0.0f;       // a sheet: darker where it is thick (a deck's heavier places)
	float baseDark = 0.0f;        // a sheet: darker in its thick middles (scud)
	float speed = 1.0f;           // times the wind aloft: scud runs faster than the deck
};

// The layers, drawn in order (the first behind). `count` 0..2; 0 = no clouds.
// The same values every frame cost nothing; a game cross-fading its sky sets
// them blended every frame.
KINJO_API void SetSkyClouds(const SkyCloudLayer* layers, int count);
// `toSun`: toward the sun (world, up is -Y). `toRim`: what thin edges glow
// toward - the sun by day, the moon at night. `glare`: how strong the sun's
// glare is in front of the clouds (0 at night), in the colour of `lit`.
KINJO_API void SetSkyCloudLight(const glm::vec3& toSun, const glm::vec3& toRim, float glare);
// The air aloft, world units a second in world x and z (TrainRails: ten to
// twenty metres a second, two or three times the wind on the ground). On the
// wind's clock (SetWindTimeScale).
KINJO_API void SetSkyCloudWind(const glm::vec2& velocityXZ);
KINJO_API int SkyCloudLayers();

// --- engine-internal -----------------------------------------------------------
class Renderer;

void UpdateSkyClouds(float realSeconds);                       // Scene3D::Update
void DrawSkyClouds(const Renderer& renderer, float radius);    // Skybox::Render, after the panorama

#endif
