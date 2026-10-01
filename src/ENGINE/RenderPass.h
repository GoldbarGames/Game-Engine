#ifndef RENDER_PASS_H
#define RENDER_PASS_H
#pragma once

#include <cstdint>
#include <functional>

// Engine-internal (not exported). The frame as a list of named passes, each
// declaring the render targets it reads and writes - a lightweight render
// graph. The GL backend runs every pass immediately, in order, and uses the
// declarations only to
//   - check ordering: a pass that reads a target nothing earlier this frame
//     wrote logs a warning (once), unless the target persists across frames;
//   - label GPU captures: each pass is a GL debug group (desktop GL 4.3+), so
//     RenderDoc / Nsight show "World", "Composite", ... instead of raw calls;
//   - describe the frame: KINJO_DUMP_FRAME=1 prints one frame's pass list.
// A Vulkan backend derives barriers and image-layout transitions from the
// same declarations.
//
// "reads" = targets the pass SAMPLES as textures (the toon outline reading
// MainDepth, lit shaders reading the shadow maps). Drawing into a target -
// including depth-tested or blended drawing onto what is already there -
// counts as a write.

enum class RenderTarget : uint8_t
{
	Backbuffer,           // the window
	ShadowMap,            // sun shadow map (depth)               - persists
	PointShadowMaps,      // point-light cube shadows (depth)     - persists (static-cached)
	MainColor,            // world framebuffer colour
	MainDepth,            // world framebuffer depth (toon outline reads it)
	CharacterMask,        // world framebuffer "is character" mask (toon outline)
	CutsceneColor,        // cutscene framebuffer colour
	PrevMainColor,        // crossfade: previous world image        - persists
	PrevCutsceneColor,    // crossfade: previous cutscene image     - persists
	Count
};

using TargetSet = uint32_t;

constexpr TargetSet Targets() { return 0; }
template <typename... Rest>
constexpr TargetSet Targets(RenderTarget first, Rest... rest)
{
	return (1u << (uint32_t)first) | Targets(rest...);
}

const char* RenderTargetName(RenderTarget target);

// Run `body` as pass `name`, declared to read `reads` and write `writes`.
// Passes may nest (a sub-pass inside a pass); the dump indents them.
void RunPass(const char* name, TargetSet reads, TargetSet writes, const std::function<void()>& body);

// Frame boundaries (Game::Render): start a fresh pass list / finish it.
void BeginFramePasses();
void EndFramePasses();

#endif
