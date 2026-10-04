#ifndef RENDER_STATE_H
#define RENDER_STATE_H
#pragma once

#include <cstdint>

class ShaderProgram;

// Engine-internal (not exported yet). Fixed-function render state as a value:
// what a Vulkan pipeline bakes in, and what the GL backend applies as a diff
// against the state it is tracking. No GL types, so the same descriptions
// will build Vulkan pipelines later.
//
// Rule: render state changes ONLY through ApplyRenderState / ScopedRenderState
// (RenderState.cpp tracks what GL currently has, so nothing ever queries GL
// with glIsEnabled/glGet* to save and restore). A raw glEnable/glDepthMask/
// glBlendFunc anywhere else desynchronises the tracking.

enum class BlendMode : uint8_t
{
	Off,        // opaque
	Alpha,      // src * a + dst * (1 - a)  (the engine's default)
	Additive,   // src + dst                 (lightning flash)
	Premultiplied,   // src + dst * (1 - src.a): a layer whose colour is already weighted by its opacity (volumetric fog)
};

enum class CompareOp : uint8_t
{
	Less,
	LessEqual,
	Always,
};

enum class CullMode : uint8_t
{
	None,
	Back,
	Front,
};

struct RenderState
{
	BlendMode blend = BlendMode::Alpha;
	bool depthTest = true;
	bool depthWrite = true;
	CompareOp depthCompare = CompareOp::Less;
	CullMode cull = CullMode::None;
	// Polygon offset (depth bias) for filled polygons; both 0 = off.
	float depthBiasFactor = 0.0f;
	float depthBiasUnits = 0.0f;

	bool operator==(const RenderState& o) const
	{
		return blend == o.blend && depthTest == o.depthTest && depthWrite == o.depthWrite
			&& depthCompare == o.depthCompare && cull == o.cull
			&& depthBiasFactor == o.depthBiasFactor && depthBiasUnits == o.depthBiasUnits;
	}
	bool operator!=(const RenderState& o) const { return !(*this == o); }
};

// A shader plus the state it draws with: the unit a Vulkan pipeline object is
// created from (vertex layout and target formats join it when the backend
// abstraction lands). On GL, binding one = UseShader + ApplyRenderState.
struct PipelineDesc
{
	const ShaderProgram* shader = nullptr;
	RenderState state;
};

// The state GL currently has (as tracked).
const RenderState& CurrentRenderState();

// Make `s` current, issuing only the GL calls for fields that differ.
void ApplyRenderState(const RenderState& s);

// Set every field unconditionally and start tracking from `s`. Called once the
// GL context exists (Game::InitOpenGL); also useful after foreign code (e.g. a
// third-party library) may have changed state behind the engine's back.
void ResetRenderState(const RenderState& s);

// Bind a pipeline: its shader, then its state.
void BindPipeline(const PipelineDesc& pipeline);

// Apply a state for the lifetime of this object, then restore whatever was
// current before - replacing the old glIsEnabled/glGet save-and-restore. Build
// the state from CurrentRenderState() and override only the fields that matter
// to the draw, so everything else carries through unchanged.
class ScopedRenderState
{
public:
	explicit ScopedRenderState(const RenderState& s);
	~ScopedRenderState();
	ScopedRenderState(const ScopedRenderState&) = delete;
	ScopedRenderState& operator=(const ScopedRenderState&) = delete;
private:
	RenderState previous;
};

// BindPipeline for the lifetime of this object; the state is restored on exit
// (the shader binding is not - every draw binds its own program anyway).
class ScopedPipeline
{
public:
	explicit ScopedPipeline(const PipelineDesc& pipeline);
private:
	ScopedRenderState state;
};

#endif
