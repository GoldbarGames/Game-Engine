#ifndef TEMPORAL_AA_H
#define TEMPORAL_AA_H
#pragma once

// Engine-internal (not exported). Temporal anti-aliasing - Phase 1.5 item 7
// (docs/RENDERING_BACKEND_PLAN.md). Linear workflow only, and only while a
// perspective 3D scene is up (2D content would just get softer).
//
// The offscreen HDR world has no MSAA, so its edges were aliased. TAA renders
// each frame with the camera shifted by a different sub-pixel offset (an
// 8-step Halton sequence) and blends it into the image accumulated over
// earlier frames, so a still view converges to a supersampled one:
//   - jitter: the world camera's projection is offset for the AO prepass and
//     the world passes only (JitterCamera ... RestoreCamera), so every world
//     draw - models, billboards, sprites, the sky - moves together;
//   - Taa pass (shaders/taa.frag): reproject each pixel into last frame's
//     image through the depth buffer and the two frames' camera matrices,
//     sample it with a Catmull-Rom filter, clip it to the colour range of
//     this frame's 3x3 neighbourhood (variance clipping in YCoCg, in a
//     compressed space so bright highlights can't smear), and blend ~10% of
//     this frame in. What the history can't explain - motion, disocclusion,
//     a lighting change - is clipped away rather than ghosting.
// It also smooths what MSAA wouldn't: cel-band edges, specular glints, the
// ambient occlusion's remaining noise, alpha-cut foliage and billboards.
//
// The toon outline is drawn into the world before TAA (so it is anti-aliased
// and doesn't shimmer with the jitter), and weather and fountain particles
// after it, unjittered, into the display copy only - fast thin streaks would
// otherwise be clipped away or smear through the history.
//
//   renderer.dat  `antialiasing taa|none`   default taa
//   KINJO_TAA=0 / =1 overrides it for one run (testing).

#include <glm/mat4x4.hpp>
#include "RenderDevice.h"

class Camera;

void LoadTemporalAASettings();   // renderer.dat / KINJO_TAA (after LoadColorSettings)

// The project wants TAA (linear workflow, setting on, shaders built). The
// frame decides the rest (a perspective 3D scene).
bool TemporalAAWanted();

// Once per frame. `active`: this frame resolves temporally. Advances the
// jitter sequence; a frame without TAA invalidates the history.
void BeginTemporalFrame(bool active, int width, int height);
bool TemporalAAActive();          // this frame (set by BeginTemporalFrame)

// Offset `camera`'s projection by this frame's sub-pixel jitter / put it back.
// Only between the two do world draws jitter. No-ops when TAA is inactive.
void JitterCamera(Camera& camera);
void RestoreCamera(Camera& camera);

// Motion vectors (shaders/motion.glsl): the world pass turns them on around
// its drawing; Scene3D's lit draws then enable the world target's motion
// attachment for themselves (Scene3DInternal::WorldDrawBuffers). Only with
// TAA resolving this frame. The frame index (0 without TAA) lets objects
// tell whether they were drawn last frame.
void SetMotionWrites(bool on);
bool MotionWritesActive();
unsigned int TemporalFrameIndex();

// Blend this frame's HDR world into the history. `depth` is the world's depth
// texture. Afterwards TemporalOutput() holds the anti-aliased image (Bloom
// and Resolve read it), and the history is ready for the next frame.
void ResolveTemporalAA(TextureHandle worldColor, TextureHandle depth, const Camera& camera,
	int screenWidth, int screenHeight);
TextureHandle TemporalOutput();   // none unless resolved this frame
// Last frame's resolved image (what this frame's resolve will read as history)
// and last frame's unjittered view-projection; none before a valid history
// (screen-space reflections look their hits up in it).
TextureHandle TemporalHistory(glm::mat4& prevViewProj);

// The toon outline drawn into the HDR world before the resolve
// (shaders/outline_world.frag), from the world's depth and character mask.
// The caller binds the Outline block first. False if it couldn't; the
// composite should then draw the outline as usual.
bool DrawWorldOutline(TextureHandle depth, TextureHandle mask);

// Draw over the anti-aliased image (weather after TAA): its colour with the
// world's depth (`depthStencil`) for depth testing. Flags the target linear.
bool BindTemporalOutputTarget(TextureHandle depthStencil);
// ...the same image with no depth attached, for a draw that samples the
// world's depth texture itself (smoke, Smoke.h): reading a texture attached to
// the framebuffer being drawn is a feedback loop.
bool BindTemporalOutputColorOnly();

// Forget the history (a camera cut: Scene3D::JumpToCamera, a scene load) -
// every split-screen view's.
void ResetTemporalHistory();

// Split screen (render/RenderViews.h): which view's history, output and last
// camera the calls above use. 0 outside split screen.
void SetTemporalView(int index);

void ReleaseTemporalAA();

#endif
