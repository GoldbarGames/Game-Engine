#ifndef KINJO_SMOKE_H
#define KINJO_SMOKE_H
#pragma once

// SMOKE AND STEAM (Smoke.cpp, shaders/smoke.*; docs/VISUAL_EFFECTS.md).
//
// Soft puffs in the 3D scene. A game emits them; the engine moves them - they
// rise, slow to the speed of the air, spread and thin out - lights them by the
// scene's sun, sky and lamps, and draws them over the world after everything
// solid, softened where they meet it so a puff never shows a straight edge
// against the ground or a train.
//
//   EmitSmoke(puff)       one puff: where, how fast, how big, how long
//   SetSmokeWind(v)       the air's own motion, world units a second
//   ClearSmoke()          all gone (a scene load does it)
//
// Units are the world's (TrainRails: metres), and up is -Y, the engine's
// convention. Colours are authored, sRGB 0..1.
//
// Drawn in its own pass ("Smoke") after the weather: over the anti-aliased
// image under TAA, reading the scene's depth for the soft edges; inside the
// world pass without TAA, depth-tested and hard-edged. `smoke 0` in
// renderer.dat or KINJO_SMOKE=0 turns the drawing off; the API keeps working.

#include "leak_check.h"
#include <glm/vec3.hpp>

struct SmokePuff
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 velocity = glm::vec3(0.0f);   // at birth, world units a second
	float radius = 0.5f;                    // at birth
	float spread = 3.0f;                    // how much the radius grows over its life
	float life = 6.0f;                      // seconds
	float opacity = 0.7f;                   // at birth, 0..1
	glm::vec3 color = glm::vec3(0.6f);      // albedo, authored sRGB (wood smoke ~0.55, steam ~0.95)
	float glow = 0.0f;                      // light it gives off, times color: sparks, a firelit plume
	float rise = 2.0f;                      // buoyancy, world units/s^2 upward (negative falls)
	float drag = 0.8f;                      // 1/s: how quickly it takes the air's speed
	float turbulence = 0.6f;                // how much it wanders, world units/s
	float steam = 0.0f;                     // 0 smoke .. 1 steam: steam glows against the sun and thins faster
};

// Adds one puff. Past the limit (4096 alive) the puff is dropped.
KINJO_API void EmitSmoke(const SmokePuff& puff);
KINJO_API void ClearSmoke();       // the flames as well
KINJO_API int SmokeCount();
KINJO_API void SetSmokeWind(const glm::vec3& windPerSecond);

// A LICK OF FLAME, or a spark (since 2026-10-07). Not a puff: a streak drawn
// along the way it is moving - a flame up from its root, a spark trailing
// behind its head - wavering and torn into tongues and wisps by the shader
// (shaders/flame.*), coloured by how hot it is (white-yellow at the heart,
// orange, a dull red at the tips) and adding light where it is. It is the
// light: nothing lights it. Moved by the same air as the smoke, in the same
// pass.
struct FlameLick
{
	glm::vec3 position = glm::vec3(0.0f);              // a flame's root; a spark's head
	glm::vec3 velocity = glm::vec3(0.0f, -2.5f, 0.0f); // at birth (up is -Y)
	float length = 0.8f;                               // at its tallest, world units
	float width = 0.3f;                                // ...and its widest
	float life = 0.6f;                                 // seconds
	float brightness = 3.0f;                           // light it gives off, times its colour
	float temperature = 0.7f;                          // 0 a dull red .. 1 a white-yellow heart
	float rise = 6.0f;                                 // buoyancy, world units/s^2 upward
	float drag = 1.5f;                                 // 1/s: how quickly it takes the air's speed
	float turbulence = 1.2f;                           // how much it wanders
	float spark = 0.0f;                                // 1: a spark - a thin hard streak, not a flame
};

// Adds one. Past the limit (3072 alive) it is dropped.
KINJO_API void EmitFlame(const FlameLick& lick);
KINJO_API int FlameCount();

// --- engine-internal -----------------------------------------------------------
class Renderer;

void UpdateSmoke(float dtSeconds);                 // Scene3D::Update
bool SmokeToDraw();                                // any puffs, and drawing is on
// Draws every puff for `renderer`'s camera. With a scene depth texture
// (sceneDepth != 0) the puffs fade where they meet the world and the depth
// test is the shader's; without one the bound framebuffer's depth tests them.
void RenderSmoke(const Renderer& renderer, unsigned int sceneDepth, int width, int height);
void LoadSmokeSettings();                          // renderer.dat `smoke`, KINJO_SMOKE

#endif
