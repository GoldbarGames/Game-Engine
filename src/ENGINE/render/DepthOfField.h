#ifndef DEPTH_OF_FIELD_H
#define DEPTH_OF_FIELD_H
#pragma once

// Engine-internal (not exported). Depth of field - Phase 1.5 item 9
// (docs/RENDERING_BACKEND_PLAN.md). Linear workflow only; a camera effect a
// scene or script turns on (cutscene close-ups), not a project-wide look.
//
// Blur size follows a thin lens: circle of confusion = aperture * (1 - focus /
// depth), so it grows quickly in front of the focus and levels off at
// `aperture` towards infinity. `aperture` is in pixels at 720p (scaled with the
// target height), capped at kMaxBlur720.
//   DofPrepare    half resolution: colour, and each pixel's signed blur size
//                 from the NEAREST of its four depths (foreground edges keep
//                 their own blur)                                (dof_prepare.frag)
//   DofBlur       a golden-angle disc of samples; a sample contributes when its
//                 own blur reaches this pixel, and something BEHIND this pixel
//                 may not blur over it by more than this pixel's own size - so a
//                 sharp subject stays sharp against a blurred background while a
//                 blurred foreground spills over it (Gustafsson's single-pass
//                 bokeh)                                         (dof_blur.frag)
//   DofComposite  full resolution: sharp where in focus, the blur elsewhere
//                                                                 (dof_composite.frag)
//
//   renderer.dat  `depthOfField 1|0`          default 1 (0 = never)
//   .scene        `dof <focus> <aperture>`    that scene's own
//   scene3d dof <focus> <aperture> [seconds] | dof char <name> <aperture> [seconds] | dof off [seconds]
//   KINJO_DOF="<focus> <aperture>" forces it for one run (testing).

#include "RenderDevice.h"
#include <string>

void LoadDepthOfFieldSettings();

// Focus distance (world units along the view) and aperture (blur in 720p
// pixels at infinity; 0 = off), reached over `seconds`. Clears any target.
void SetDepthOfField(float focus, float aperture, float seconds);
// Follow a character with the focus (Scene3D resolves the distance each
// frame through SetDepthOfFieldFocus); aperture as above.
void SetDepthOfFieldTarget(const std::string& character, float aperture, float seconds);
const std::string& DepthOfFieldTarget();
void SetDepthOfFieldFocus(float focus);        // the target's distance this frame
void GetSceneDepthOfField(float& focus, float& aperture);   // as set (for saving; aperture 0 = off)

bool DepthOfFieldActive();   // this frame blurs (linear, allowed, aperture > 0)

// The passes, on `source` (the world image this frame) with the world's
// depth. Afterwards DepthOfFieldOutput() holds the result.
void ApplyDepthOfField(TextureHandle source, TextureHandle depth, float nearPlane, float farPlane,
	int width, int height);
TextureHandle DepthOfFieldOutput();

void ReleaseDepthOfField();

#endif
