#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H
#pragma once

// Engine-internal (not exported). Screen-space ambient occlusion - Phase 1.5
// item 6 (docs/RENDERING_BACKEND_PLAN.md). Linear workflow only.
//
// GTAO (Jimenez et al. 2016, after Intel's XeGTAO) in fragment shaders, since
// WebGL2 has no compute. Each frame with a lit 3D scene:
//   AoPrepass   the opaque Scene3D models again, writing linear view depth
//               (R32F) and view-space FACE normals (RGBA8) - not the mesh's,
//               which often disagree with the surface        (ao_prepass.*)
//   Gtao        a horizon search per pixel: 3 slice directions x 6 steps per
//               side, rotated by a 4x4 tile of offsets       (gtao.frag)
//   AoDenoise   a 4x4 depth- and normal-aware average that removes that tile
//               pattern                                       (ao_denoise.frag)
// Lit shaders read the result through shaders/ao.glsl. It darkens only the
// AMBIENT light (the sky's light with IBL, or the flat `ambient` colour) and
// sky reflections, never direct light, as the plan asks. A fragment the
// prepass didn't draw (glass, water, anything not a Scene3D model) is left
// unoccluded: ao.glsl compares its depth with the prepass's.
//
//   renderer.dat  `ao <strength 0..1>`   default 1; 0 = off
//                 `aoRadius <world units>`  default 60
//   .scene        `ao <strength> [radius]`  that scene's own values
//   KINJO_AO=0 / =1, KINJO_AO_RADIUS=<r> override for one run (testing);
//   KINJO_AO_DEBUG=1 shows the occlusion itself on lit surfaces.

#include <glm/mat4x4.hpp>
#include "RenderDevice.h"

void LoadAmbientOcclusionSettings();   // renderer.dat / KINJO_AO* (after LoadColorSettings)

// The project and the current scene want occlusion (linear workflow, strength
// > 0, the shaders built). The frame decides the rest (a perspective 3D scene).
bool AmbientOcclusionWanted();

// The prepass: sizes the targets to the world's, binds and clears them, and
// returns the programs to draw opaque geometry with - `program` takes the
// per-draw `model` matrix (as scene3d.vert does), `instancedProgram` the
// per-instance matrices of Mesh::SetInstances (as scene3d_instanced.vert);
// both cut out by the albedo's alpha and the Material block's uv tiling.
// False if occlusion can't run; nothing is then bound.
bool BeginAoPrepass(int width, int height, unsigned int& program, unsigned int& instancedProgram);
void EndAoPrepass(int screenWidth, int screenHeight);   // restores the window target + viewport

// Occlusion from the prepass, for the camera that drew it, into the map lit
// shaders read. Each runs as its own declared pass (Game::Render).
void ComputeAmbientOcclusion(const glm::mat4& projection);
void DenoiseAmbientOcclusion(int screenWidth, int screenHeight);   // restores the viewport

bool AmbientOcclusionActive();          // this frame's map is valid (reset by BeginAoFrame)

// The prepass also serves screen-space reflections (render/Reflections.h):
// it runs when either wants it, and its targets are readable after it.
bool PrepassWanted();
TextureHandle PrepassViewDepth();       // R32F linear view depth (none before this frame's prepass)
TextureHandle PrepassViewNormal();      // RGBA8 view-space face normal, a = roughness
void BeginAoFrame();                    // once per frame, before anything else

// Per lit draw (Scene3D::ApplyLighting): the AmbientOcclusion block, and the
// maps on texture units 12-13 when active. Always safe to call.
void BindAmbientOcclusion(unsigned int program);

// The current scene's values: < 0 = the project default.
void SetSceneAO(float strength, float radius);
void GetSceneAO(float& strength, float& radius);
void GetProjectAO(float& strength, float& radius);   // renderer.dat `ao` / `aoRadius`

// Show the occlusion itself on lit surfaces (KINJO_AO_DEBUG; the 3D editor's
// LOOK panel toggles it at runtime).
void SetAmbientOcclusionDebugView(bool on);
bool AmbientOcclusionDebugView();

void ReleaseAmbientOcclusion();

#endif
