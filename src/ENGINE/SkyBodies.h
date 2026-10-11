#ifndef KINJO_SKY_BODIES_H
#define KINJO_SKY_BODIES_H
#pragma once

// THE SUN AND THE MOON IN THE SKY (SkyBodies.cpp, shaders/sky.*,
// shaders/sky_bodies.glsl; docs/VISUAL_EFFECTS.md).
//
// Drawn live with the sky's panorama, wherever the game puts them, so they can
// move with the hour:
//   * the sun's disc and the glare round it;
//   * the sky's glow toward the sun, and at dawn and dusk the warm horizon
//     under it, with the pink band and the earth's blue shadow low on the far
//     side;
//   * the moon's disc with its face, lit on the side toward the sun - so its
//     phase follows from where the two are - and its glow.
// The panorama is then painted without any of them: a sun painted in stays
// where it was painted.
//
//   SetSkySun(toSun, look)      where the sun is, and how it shows
//   SetSkyMoon(toMoon, look)    where the moon is, and how it shows
//   ClearSkyBodies()            back to the panorama alone (the default)
//
// Off until a game sets one. Directions are world (up is -Y), toward the body;
// colours are authored sRGB 0..1, in the panorama's terms. A game that cross-
// fades its skies blends the looks to match, as it does its clouds (SkyClouds.h).
// The scene's light is separate: the game aims Scene3D::SetSunDirection too.
//
// Image-based lighting captures the sun's glow and the dawn colours with the
// panorama, and captures again as the sun moves (each half degree) or its look
// changes. The disc, the glare and the moon are left out of it.

#include "leak_check.h"
#include <glm/vec3.hpp>

class Texture;

struct SkySunLook
{
	glm::vec3 colour = glm::vec3(1.0f, 0.97f, 0.90f);   // the disc and its glare
	float disc = 4.0f;            // the disc's brightness (0: none): far over the sky's, so it blooms
	float radius = 0.0056f;       // the disc's angular radius, radians (life: 0.0047)
	float glare = 0.6f;           // round the disc, in the eye and the air: in front of clouds too
	glm::vec3 glow = glm::vec3(1.0f, 0.94f, 0.82f);     // the sky paler and brighter toward the sun...
	float glowWide = 0.16f;       // ...across half the sky
	float glowNear = 0.10f;       // ...and close round it
	float wideFalloff = 2.5f;     // how fast each falls away (larger: tighter)
	float nearFalloff = 20.0f;
	glm::vec3 horizon = glm::vec3(1.0f, 0.62f, 0.32f);  // dawn and dusk: the horizon under the sun...
	float horizonAmount = 0.0f;   // ...this far toward that colour (0 by day)
	float dusk = 0.0f;            // the pink band and the earth's shadow opposite the sun (0..1)
};

struct SkyMoonLook
{
	Texture* face = nullptr;      // its face: albedo, the disc filling the square (north up); null: plain
	glm::vec3 colour = glm::vec3(1.08f, 1.05f, 0.97f);  // times the face
	float brightness = 1.0f;      // 0: not drawn
	float radius = 0.0096f;       // angular radius, radians (life: 0.0045)
	glm::vec3 north = glm::vec3(0.0f, -1.0f, 0.0f);     // toward the celestial pole: which way its north points
	float earthshine = 0.03f;     // how much of its dark part shows
	glm::vec3 glowColour = glm::vec3(0.62f, 0.70f, 0.92f);
	float glow = 1.0f;            // the glow round it, times how full it is (0 by day)
};

KINJO_API void SetSkySun(const glm::vec3& toSun, const SkySunLook& look);
KINJO_API void SetSkyMoon(const glm::vec3& toMoon, const SkyMoonLook& look);
KINJO_API void ClearSkyBodies();

// --- engine-internal -----------------------------------------------------------
class Renderer;

// Skybox::Render: draws the panorama (cross-fade and tint) with the sun and the
// moon, in place of its own passes. False when no game has set them (or the
// camera is orthographic): the skybox draws as it always has.
bool DrawSkyWithBodies(const Renderer& renderer, float radius, Texture* sky, Texture* next, float blend,
	const glm::vec3& tint);
// Image-based lighting (render/Environment.cpp): whether the sun's air is in the
// sky, its block for the capture shader, and a number that changes whenever the
// capture should be made again.
bool SkySunInSky();
void BindSkyBlock();
int SkyBodiesCaptureVersion();

#endif
