// Render-state front end: the tracked state and the diff-apply live in the
// active RenderDevice (render/gl/GLDevice.cpp for GL); this file is
// backend-neutral.

#include "RenderState.h"
#include "Shader.h"
#include "render/RenderDevice.h"

const RenderState& CurrentRenderState()
{
	return Device().CurrentState();
}

void ApplyRenderState(const RenderState& s)
{
	Device().ApplyState(s, false);
}

void ResetRenderState(const RenderState& s)
{
	Device().ApplyState(s, true);
}

void BindPipeline(const PipelineDesc& pipeline)
{
	if (pipeline.shader != nullptr)
		pipeline.shader->UseShader();
	ApplyRenderState(pipeline.state);
}

ScopedRenderState::ScopedRenderState(const RenderState& s)
	: previous(CurrentRenderState())
{
	ApplyRenderState(s);
}

ScopedRenderState::~ScopedRenderState()
{
	ApplyRenderState(previous);
}

ScopedPipeline::ScopedPipeline(const PipelineDesc& pipeline)
	: state(pipeline.state)
{
	if (pipeline.shader != nullptr)
		pipeline.shader->UseShader();
}
