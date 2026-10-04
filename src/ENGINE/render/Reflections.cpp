// Screen-space reflections - see Reflections.h. Backend-agnostic: GPU work
// goes through RenderDevice, shaders through ShaderProgram.

#include "Reflections.h"
#include "AmbientOcclusion.h"
#include "ColorPipeline.h"
#include "TemporalAA.h"
#include "../Shader.h"
#include "../RenderState.h"
#include "../globals.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace
{
	bool projectOn = true;        // renderer.dat `reflections`
	int forced = -1;              // KINJO_SSR
	float maxDistance = 1500.0f;  // renderer.dat `reflectionDistance`
	const float kThickness = 25.0f;     // world units a surface is assumed to be deep
	const float kRoughCutoff = 0.6f;    // rougher surfaces don't reflect the screen

	bool active = false;
	unsigned int frameIndex = 0;
	int width = 0, height = 0;
	TextureHandle reflection;
	FramebufferHandle fbo;
	VertexArrayHandle emptyVao;
	ShaderProgram* shader = nullptr;
	bool failed = false;
	struct Locs { int viewToPrevClip = -1, proj = -1, params = -1, size = -1, nearPlane = -1,
		depth = -1, normal = -1, history = -1; };
	Locs locs;

	bool EnsureShader()
	{
		if (failed)
			return false;
		if (shader != nullptr)
			return true;
		shader = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/ssr.frag");
		shader->SetNameString("ssr");
		if (shader->GetID() == 0)
		{
			std::cout << "ERROR: screen-space reflection shader failed to build; reflections disabled" << std::endl;
			delete shader;
			shader = nullptr;
			failed = true;
			return false;
		}
		const unsigned int id = shader->GetID();
		RenderDevice& device = Device();
		locs.viewToPrevClip = ShaderProgram::DrawUniformLocation(id, "viewToPrevClip");
		locs.proj = ShaderProgram::DrawUniformLocation(id, "proj");
		locs.params = ShaderProgram::DrawUniformLocation(id, "params");
		locs.size = ShaderProgram::DrawUniformLocation(id, "size");
		locs.nearPlane = ShaderProgram::DrawUniformLocation(id, "nearPlane");
		locs.depth = device.UniformLocation(ProgramHandle(id), "viewDepth");
		locs.normal = device.UniformLocation(ProgramHandle(id), "viewNormal");
		locs.history = device.UniformLocation(ProgramHandle(id), "history");
		return true;
	}

	void EnsureTarget(int w, int h)
	{
		if (reflection && w == width && h == height)
			return;
		RenderDevice& device = Device();
		if (reflection)
			device.DestroyTexture(reflection);
		if (fbo)
			device.DestroyFramebuffer(fbo);
		TextureDesc desc;
		desc.format = TextureFormat::RGBA16F;
		desc.width = w;
		desc.height = h;
		desc.filter = TextureFilter::Nearest;   // read with texelFetch
		desc.wrap = TextureWrap::ClampToEdge;
		reflection = device.CreateTexture(desc);
		fbo = device.CreateFramebuffer();
		device.AttachTexture(fbo, Attachment::Color0, reflection);
		if (!emptyVao)
			emptyVao = device.CreateVertexArray();
		device.BindFramebuffer(FramebufferHandle());
		width = w;
		height = h;
	}
}

void LoadReflectionSettings()
{
	auto config = GetMapStringsFromFile(RendererConfigPath());
	projectOn = !(config.count("reflections") > 0 && config["reflections"] == "0");
	if (config.count("reflectionDistance") > 0)
	{
		try { maxDistance = std::max(std::stof(config["reflectionDistance"]), 50.0f); }
		catch (...) {}
	}
	if (const char* e = std::getenv("KINJO_SSR"))
		forced = (e[0] == '0') ? 0 : (e[0] == '1') ? 1 : -1;
}

bool ReflectionsWanted()
{
	const bool on = (forced >= 0) ? (forced == 1) : projectOn;
	return on && LinearWorkflow() && TemporalAAWanted() && !failed;
}

void BeginReflectionsFrame()
{
	active = false;
}

void ComputeReflections(const glm::mat4& view, const glm::mat4& projection, int w, int h)
{
	active = false;
	glm::mat4 prevViewProj;
	const TextureHandle history = TemporalHistory(prevViewProj);
	const TextureHandle depth = PrepassViewDepth();
	const TextureHandle normal = PrepassViewNormal();
	if (!ReflectionsWanted() || !history || !depth || !normal || w <= 0 || h <= 0 || !EnsureShader())
		return;
	EnsureTarget(w, h);

	RenderDevice& device = Device();
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Off;
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	device.BindFramebuffer(fbo);
	device.SetViewport(0, 0, w, h);
	shader->UseShader();
	device.BindTexture(0, depth);
	device.BindTexture(1, normal);
	device.BindTexture(2, history);
	device.SetUniform(locs.depth, 0);
	device.SetUniform(locs.normal, 1);
	device.SetUniform(locs.history, 2);
	device.SetUniform(locs.viewToPrevClip, prevViewProj * glm::inverse(view));
	device.SetUniform(locs.proj, glm::vec4(projection[0][0], projection[1][1], projection[2][0], projection[2][1]));
	device.SetUniform(locs.params, glm::vec4(maxDistance, kThickness, kRoughCutoff, (float)(frameIndex++ % 64)));
	device.SetUniform(locs.size, glm::vec2((float)w, (float)h));
	// Near plane from the projection itself: n = P[3][2] / (P[2][2] - 1).
	device.SetUniform(locs.nearPlane, projection[3][2] / (projection[2][2] - 1.0f));
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	device.BindFramebuffer(FramebufferHandle());
	active = true;
}

bool ReflectionsActive()
{
	return active;
}

TextureHandle ReflectionTexture()
{
	return active ? reflection : TextureHandle();
}

void ReleaseReflections()
{
	RenderDevice& device = Device();
	if (reflection)
		device.DestroyTexture(reflection);
	if (fbo)
		device.DestroyFramebuffer(fbo);
	if (emptyVao)
		device.DestroyVertexArray(emptyVao);
	delete shader;
	shader = nullptr;
	failed = false;
	active = false;
	width = height = 0;
}
