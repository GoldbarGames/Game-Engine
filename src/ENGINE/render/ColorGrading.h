#ifndef COLOR_GRADING_H
#define COLOR_GRADING_H
#pragma once

// Engine-internal (not exported). Colour grading with a lookup table - Phase
// 1.5 item 9 (docs/RENDERING_BACKEND_PLAN.md). Linear workflow only: the
// Resolve pass applies it after tonemapping, to the final display colours.
//
// A LUT is a "strip" PNG, N*N wide and N tall (N = 16, 32 or 64): N slices
// left to right for blue, red left to right inside a slice, green top to
// bottom. Grade one in any image editor: take a screenshot of the game, paste
// the neutral LUT (utils/templates/project/data/luts/neutral32.png) into it,
// apply colour adjustments to the whole image, and cut the strip back out.
//
//   renderer.dat  `colorGrade <png>`          the project's look (none by default)
//                 `colorGradeStrength <0..1>` default 1
//   .scene        `grade <png> [strength]`    that scene's own look
//   scene3d grade <png|none|default> [strength] [seconds] - fades between looks
//   KINJO_GRADE=<png> overrides everything for one run (testing).

#include "RenderDevice.h"
#include <string>

void LoadColorGradeSettings();

// The current scene's look: an empty path = the project's, "none" = off. With
// seconds > 0 it cross-fades from the look on screen.
void SetSceneColorGrade(const std::string& path, float strength, float seconds);
void GetSceneColorGrade(std::string& path, float& strength);   // as set (for saving)
void GetProjectColorGrade(std::string& path, float& strength); // renderer.dat (path "" = none)

// What the Resolve pass applies this frame. False = no grading.
struct ColorGradeState
{
	TextureHandle lut;
	float lutSize = 0.0f;
	float strength = 0.0f;
	TextureHandle previous;   // the look fading out (none = the ungraded image)
	float previousSize = 0.0f;
	float previousStrength = 0.0f;
	float blend = 1.0f;       // from `previous` toward `lut`, 0..1
};
bool CurrentColorGrade(ColorGradeState& out);

void ReleaseColorGrading();

#endif
