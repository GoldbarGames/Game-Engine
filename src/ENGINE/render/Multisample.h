#ifndef MULTISAMPLE_H
#define MULTISAMPLE_H
#pragma once

// Engine-internal (not exported). Multisample anti-aliasing (MSAA) for the
// world, in either colour mode: renderer.dat `msaa 0|2|4|8` (default 0 = off),
// KINJO_MSAA=<n> for one run.
//
// With it on, the World pass draws into multisampled renderbuffers - colour
// (RGBA8, or RGBA16F in the linear workflow), the character mask, the motion
// vectors under temporal anti-aliasing, and depth - instead of the world
// target, and an MsaaResolve pass straight after blits them into that
// target's single-sample textures. Every later pass reads what it always did.
// Renderbuffers rather than multisampled textures, because WebGL2 has none of
// those. The crossfade capture (Game::Render) stays single-sampled: it is only
// a fade.
//
// What it doesn't smooth:
//   - cut-out edges: textures with transparent holes `discard`, and there is
//     no alpha-to-coverage;
//   - the toon outline, which is found in the resolved depth;
//   - very bright edges in the linear workflow, averaged before tonemapping.
// With temporal anti-aliasing on as well it is redundant (TAA smooths edges
// already) and costs the multisampled memory: at 1920x1080 with 4x, about
// 75 MB in gamma mode and 165 MB in linear mode with motion vectors.

#include "RenderDevice.h"

// renderer.dat `msaa` and KINJO_MSAA: at startup, and again when the editor's
// PROJECT page changes renderer.dat (the buffers follow on the next frame).
void LoadMultisampleSettings();

// Make sure the multisampled world target exists for this frame's world
// target: `width` x `height`, colour in `colorFormat`, motion vectors when
// `motion`. False when MSAA is off, or this GPU can't multisample these
// formats (the world then draws into its own target as before).
bool EnsureMultisampleWorld(int width, int height, TextureFormat colorFormat, bool motion);
// Bind it (after EnsureMultisampleWorld said yes), in place of the world target.
void BindMultisampleWorld();
// Resolve it into `worldTarget`, the framebuffer it stood in for: colour, the
// mask, the motion vectors and depth.
void ResolveMultisampleWorld(FramebufferHandle worldTarget);
// The samples the world is drawn with now (0 = no MSAA).
int MultisampleSamples();

void ReleaseMultisample();

#endif
