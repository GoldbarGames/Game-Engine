#ifndef COLOR_PIPELINE_H
#define COLOR_PIPELINE_H
#pragma once

// Engine-internal (not exported). The project's colour pipeline - Phase 1.5
// item 1, the linear workflow (docs/RENDERING_BACKEND_PLAN.md).
//
// Off (the default, every existing game): nothing changes.
//
// On (`linearLighting 1` in data/config/renderer.dat, or KINJO_LINEAR=1):
//   - The world (World pass, crossfade capture) renders into a linear RGBA16F
//     target that shares the main framebuffer's depth and character mask. A
//     Resolve pass then encodes it back to sRGB into the main framebuffer's
//     colour, so the composite, the toon outline, game post shaders and
//     screenshots are untouched.
//   - Scene3D's colour art (model albedo, character billboards, seasonal
//     swaps) loads as sRGB textures, so lit shaders sample linear values; light
//     and material colours are converted to linear on the CPU (SceneColor).
//     Scalars - intensities, ranges, NdotL - are already linear.
//   - Linear-aware shaders (those including shaders/target.glsl: scene3d,
//     billboard3d, flash) write linear radiance. EVERY OTHER fragment shader -
//     the engine's sprite family, each game's own shaders.dat set, embedded
//     shaders - is treated as gamma-space art: its source is wrapped so that,
//     in a linear target, its output is clamped like an 8-bit target would and
//     converted to linear. 2D content in the world therefore keeps its exact
//     look; only its blending with other content happens in linear space.
//   - Scene3D's linear-aware shaders always come from the engine's shaders/
//     folder (a game's old copies predate this and would be decoded twice).

#include <string>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "RenderDevice.h"

class FrameBuffer;

// Read the setting (once the render device exists, before shaders compile).
void LoadColorSettings();

// Is the linear workflow on for this run?
bool LinearWorkflow();

// The exact sRGB transfer curve (CPU side).
glm::vec3 SrgbToLinear(const glm::vec3& srgb);

// An authored colour (light, material tint, clear colour) as lit shaders
// should receive it: converted to linear when the workflow is on, else as is.
glm::vec3 SceneColor(const glm::vec3& srgb);
glm::vec4 SceneColor(const glm::vec4& srgb);   // alpha unchanged

// Tell shaders whether the bound render target is linear (the KinjoTarget
// block). Bound to false once at startup and after every world target.
void SetTargetLinear(bool linear);

// --- world target ---------------------------------------------------------
// Bind where the world draws for `fb`: the linear HDR target when the workflow
// is on (and flag shaders), else fb's own framebuffer.
void BindWorldTarget(FrameBuffer& fb);
// Linear workflow only: expose, tonemap and encode the HDR world into fb's
// colour attachment.
void ResolveWorldTarget(FrameBuffer& fb);
// Drop the HDR targets (the main framebuffers were recreated).
void ReleaseWorldTargets();
// The HDR world colour (none unless linear and allocated), for passes that
// work on the world image before Resolve (temporal anti-aliasing).
TextureHandle WorldHdrTexture();
// The world targets' motion-vector attachment (colour attachment 2): present
// only when temporal anti-aliasing is wanted.
TextureHandle WorldMotionTexture();
// Bind the HDR world colour ALONE (no depth or mask attached), so a pass can
// draw over the world while sampling the world's depth (the toon outline
// under temporal anti-aliasing). Flags the target linear. False if not linear.
bool BindWorldColorOnly();
// Bloom and Resolve read this instead of the world colour while it is set
// (the temporal anti-aliasing output); TextureHandle() = the world colour.
void SetWorldSourceOverride(TextureHandle texture);
// Everything, at shutdown (while the context is alive).
void ReleaseColorPipeline();

// --- exposure and tonemapping (Phase 1.5 item 2) ----------------------------
// Applied by the Resolve pass, so only with the linear workflow on.
//   renderer.dat  `tonemap none|agx|agx_punchy|aces` (default agx_punchy)
//                 `exposure <multiplier>`             (default 1.0)
//   .scene        `exposure <multiplier>` - that scene's own value
//   KINJO_TONEMAP / KINJO_EXPOSURE override both for one run (testing).
// Exposure multiplies the world's linear light before the tonemapper.
enum class Tonemapper { None = 0, AgX = 1, AgXPunchy = 2, ACES = 3 };
Tonemapper ActiveTonemapper();
const char* TonemapperName(Tonemapper t);
// The current scene's exposure: 0 = the project default. With seconds > 0 it
// fades there from the current value.
void SetSceneExposure(float multiplier, float seconds = 0.0f);
float SceneExposure();       // as authored/set (0 = project default), for saving
float EffectiveExposure();   // what the Resolve pass applies this frame

// --- bloom (Phase 1.5 item 3) ------------------------------------------------
// A downsample/upsample mip chain over the HDR world (shaders/bloom_down.frag,
// bloom_up.frag), blended into the image by the Resolve pass before exposure
// and tonemapping. Linear workflow only.
//   renderer.dat  `bloom <strength>`  0..1, default 0.04; 0 = off
//   .scene        `bloom <strength>`  that scene's own value
//   KINJO_BLOOM overrides both for one run (testing).
// Strength is the fraction of the image replaced by its blur, so flat areas
// keep their brightness; light much brighter than its surroundings (emissives
// above 1, specular glints) is what visibly glows.
bool BloomEnabled();            // this frame's strength > 0 (and linear on)
void BloomWorldTarget();        // build the chain from the HDR world, before Resolve
// The current scene's bloom strength: < 0 = the project default.
void SetSceneBloom(float strength);
float SceneBloom();             // as authored/set (< 0 = project default), for saving
float EffectiveBloom();         // what Resolve applies this frame

// --- shader adaptation (Shader.cpp) ---------------------------------------
// The fragment source to compile: unchanged unless the workflow is on and the
// shader is not linear-aware, in which case its output is wrapped (`adapted`).
std::string AdaptFragmentShader(const std::string& source, bool& adapted);
// True for the shader files that must come from the engine when the workflow
// is on (Scene3D's linear-aware set), whatever the game's data/shaders holds.
bool RequiresEngineShader(const std::string& path);

#endif
