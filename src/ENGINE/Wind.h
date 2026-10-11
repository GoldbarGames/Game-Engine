#ifndef KINJO_WIND_H
#define KINJO_WIND_H
#pragma once

// WIND (Wind.cpp, shaders/wind.glsl; docs/VISUAL_EFFECTS.md).
//
// The air the scene's plants sway in. A material with a `wind` line in
// materials.txt bends with it - trees lean and rock, leaves and grass blades
// flutter - in the world pass, the ambient-occlusion prepass and the motion
// vectors (TAA), so a swaying crown neither smears nor loses its occlusion.
//
//   SetWind(toward, strength)        which way it blows, and how hard (0..1)
//   SetWindGusts(length, speed)      how far apart the gusts are, how fast they cross
//   SetWindTimeScale(scale)          its clock against real time
//
// Units are the world's (TrainRails: metres), and up is -Y. The default is a
// light breeze, so a material with `wind` sways without any code.
//
// HOW MUCH EACH VERTEX MOVES. `wind <sway> [height]`: `sway` is how far the
// top of the thing leans in a fresh breeze (strength 1), in world units. Each
// vertex is weighted, 0 at the root to 1 at the top:
//   * with a `height`: by its own height above the model's origin (local -Y),
//     over that height, squared - an imported tree or bush, nothing else needed;
//   * without one: by the mesh's TANGENT slot, which a mesh built for it fills
//     (x = how far it bends, 0 root .. 1 top; y = how much it flutters, a leaf or
//     a blade's tip; z = its phase, 0..1, so neighbours don't move in step).
//     Like `splat`, that is for meshes made in code (TrainRails' woods and
//     grass): an imported model's tangents are directions, not weights.
//
// Shadows are cast and looked up where the plant stands at rest, so a still
// shadow map stays cached (the sun's cascades redraw only when something
// moves) and a swaying leaf does not shade itself in patches.

#include "leak_check.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

// `towardXZ`: the way it blows toward, in world x and z (need not be unit
// length). `strength`: 0 calm .. 1 a fresh breeze .. 2 a gale.
KINJO_API void SetWind(const glm::vec2& towardXZ, float strength);
// `gustLength`: world units between gusts as they sweep across the country
// (default 40); `gustSpeed`: how fast they travel, world units a second (8).
KINJO_API void SetWindGusts(float gustLength, float gustSpeed);
// The wind's clock runs this many times real time: a game that runs its world
// faster than life (TrainRails' time scale) keeps the plants in step with it.
KINJO_API void SetWindTimeScale(float scale);
KINJO_API float WindClock();          // seconds on the wind's clock
KINJO_API glm::vec2 WindToward();     // unit, world x and z
KINJO_API float WindStrength();

// --- engine-internal -----------------------------------------------------------

void UpdateWind(float realSeconds);   // once a frame (Scene3D::Update)
float WindTimeScale();                // the clock's rate (SkyClouds.cpp drifts on it)
// For the Scene block (shaders/scene.glsl): x, y = toward (world x, z), z =
// strength, w = the clock (wrapped); and x = last frame's clock, y = the gust
// length, z = how far the gusts have travelled (wrapped at a gust length), w =
// the same last frame.
glm::vec4 WindBlockParams();
glm::vec4 WindBlockParams2();

#endif
