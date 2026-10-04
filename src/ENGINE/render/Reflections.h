#ifndef REFLECTIONS_H
#define REFLECTIONS_H
#pragma once

// Engine-internal (not exported). Screen-space reflections - Phase 1.5 item 9
// (docs/RENDERING_BACKEND_PLAN.md). Linear workflow with temporal
// anti-aliasing (it reflects last frame's image, from TAA's history).
//
// Before the world pass, every pixel of a glossy surface (roughness from the
// ambient-occlusion prepass, which then runs for reflections too) marches its
// reflected view ray through the prepass's depth; where the ray hits, the hit
// point is reprojected into last frame's anti-aliased image and that colour is
// the reflection (shaders/ssr.frag). Lit shaders (ao.glsl ScreenReflection)
// blend it over their sky reflection - or, indoors with no sky, use it alone -
// weighted by the same Fresnel and specular terms; misses keep the sky.
// Confidence fades with roughness, distance, the screen edge and rays turning
// back towards the camera, so reflections never end in a hard seam.
//
//   renderer.dat  `reflections 1|0` (default 1), `reflectionDistance <world units>`
//   KINJO_SSR=0 / =1 overrides for one run (testing).

#include "RenderDevice.h"
#include <glm/mat4x4.hpp>

void LoadReflectionSettings();
bool ReflectionsWanted();    // the project wants them (linear, TAA wanted, setting on)

void BeginReflectionsFrame();   // once per frame
// The march (after the prepass, before the world). Needs TAA's history from
// last frame; does nothing without it.
void ComputeReflections(const glm::mat4& view, const glm::mat4& projection, int width, int height);
bool ReflectionsActive();         // this frame's map is valid
TextureHandle ReflectionTexture();

void ReleaseReflections();

#endif
