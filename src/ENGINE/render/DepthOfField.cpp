// Depth of field - see DepthOfField.h. Backend-agnostic: GPU work goes
// through RenderDevice, shaders through ShaderProgram.

#include "DepthOfField.h"
#include "ColorPipeline.h"
#include "../Shader.h"
#include "../RenderState.h"
#include "../globals.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace
{
	const float kMaxBlur720 = 24.0f;   // largest blur, in pixels at 720p

	// --- settings and state -------------------------------------------------
	bool allowed = true;           // renderer.dat `depthOfField`
	bool forced = false;           // KINJO_DOF
	float forcedFocus = 0.0f, forcedAperture = 0.0f;
	std::string target;            // character the focus follows ("" = none)

	// Focus and aperture fade from `from` to `to` over `seconds`.
	float fromFocus = 500.0f, toFocus = 500.0f;
	float fromAperture = 0.0f, toAperture = 0.0f;
	float seconds = 0.0f;
	std::chrono::steady_clock::time_point start;

	float Progress()
	{
		if (seconds <= 0.0f)
			return 1.0f;
		const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() / seconds;
		return std::min(std::max(t, 0.0f), 1.0f);
	}
	float CurrentFocus()
	{
		return forced ? forcedFocus : fromFocus + (toFocus - fromFocus) * Progress();
	}
	float CurrentAperture()
	{
		return forced ? forcedAperture : fromAperture + (toAperture - fromAperture) * Progress();
	}

	void StartFade(float focus, float aperture, float fadeSeconds)
	{
		fromFocus = CurrentFocus();
		fromAperture = CurrentAperture();
		toFocus = std::max(focus, 1.0f);
		toAperture = std::max(aperture, 0.0f);
		seconds = fadeSeconds;
		start = std::chrono::steady_clock::now();
		if (fadeSeconds <= 0.0f)
		{
			fromFocus = toFocus;
			fromAperture = toAperture;
		}
	}

	// --- resources ----------------------------------------------------------
	int width = 0, height = 0;
	TextureHandle halfColor, halfBlur, output;
	FramebufferHandle halfColorFbo, halfBlurFbo, outputFbo;
	VertexArrayHandle emptyVao;
	ShaderProgram* prepare = nullptr;
	ShaderProgram* blur = nullptr;
	ShaderProgram* composite = nullptr;
	bool failed = false;
	bool produced = false;   // this frame's output exists

	struct Locs { int lens = -1, planes = -1, size = -1, maxBlur = -1, color = -1, depth = -1, blurTex = -1; };
	Locs prepareLocs, blurLocs, compositeLocs;

	// sizeName / imageName: what this pass calls its size and its main input.
	ShaderProgram* Build(const char* frag, const char* name, const char* sizeName, const char* imageName, Locs& l)
	{
		ShaderProgram* s = new ShaderProgram(-1, "data/shaders/resolve.vert", frag);
		s->SetNameString(name);
		if (s->GetID() == 0)
		{
			delete s;
			return nullptr;
		}
		const unsigned int id = s->GetID();
		l.lens = ShaderProgram::DrawUniformLocation(id, "lens");
		l.planes = ShaderProgram::DrawUniformLocation(id, "planes");
		l.size = ShaderProgram::DrawUniformLocation(id, sizeName);
		l.maxBlur = ShaderProgram::DrawUniformLocation(id, "maxBlur");
		l.color = Device().UniformLocation(ProgramHandle(id), "srcColor");
		l.depth = Device().UniformLocation(ProgramHandle(id), "srcDepth");
		l.blurTex = Device().UniformLocation(ProgramHandle(id), imageName);
		return s;
	}

	bool EnsureShaders()
	{
		if (failed)
			return false;
		if (composite != nullptr)
			return true;
		prepare = Build("data/shaders/dof_prepare.frag", "dof_prepare", "dstSize", "srcColor", prepareLocs);
		blur = Build("data/shaders/dof_blur.frag", "dof_blur", "size", "halfColor", blurLocs);
		composite = Build("data/shaders/dof_composite.frag", "dof_composite", "size", "blurColor", compositeLocs);
		if (prepare == nullptr || blur == nullptr || composite == nullptr)
		{
			std::cout << "ERROR: depth-of-field shaders failed to build; depth of field disabled" << std::endl;
			delete prepare;
			delete blur;
			delete composite;
			prepare = blur = composite = nullptr;
			failed = true;
			return false;
		}
		return true;
	}

	void ReleaseTargets()
	{
		RenderDevice& device = Device();
		for (TextureHandle* t : { &halfColor, &halfBlur, &output })
			if (*t)
				device.DestroyTexture(*t);
		for (FramebufferHandle* f : { &halfColorFbo, &halfBlurFbo, &outputFbo })
			if (*f)
				device.DestroyFramebuffer(*f);
		width = height = 0;
	}

	void EnsureTargets(int w, int h)
	{
		if (output && w == width && h == height)
			return;
		ReleaseTargets();
		RenderDevice& device = Device();
		auto make = [&](int tw, int th, TextureHandle& tex, FramebufferHandle& fbo)
		{
			TextureDesc desc;
			desc.format = TextureFormat::RGBA16F;
			desc.width = tw;
			desc.height = th;
			desc.filter = TextureFilter::Linear;
			desc.wrap = TextureWrap::ClampToEdge;
			tex = device.CreateTexture(desc);
			fbo = device.CreateFramebuffer();
			device.AttachTexture(fbo, Attachment::Color0, tex);
		};
		make(std::max(w / 2, 1), std::max(h / 2, 1), halfColor, halfColorFbo);
		make(std::max(w / 2, 1), std::max(h / 2, 1), halfBlur, halfBlurFbo);
		make(w, h, output, outputFbo);
		if (!emptyVao)
			emptyVao = device.CreateVertexArray();
		device.BindFramebuffer(FramebufferHandle());
		width = w;
		height = h;
	}
}

void LoadDepthOfFieldSettings()
{
	auto config = GetMapStringsFromFile(RendererConfigPath());
	allowed = !(config.count("depthOfField") > 0 && config["depthOfField"] == "0");
	if (const char* e = std::getenv("KINJO_DOF"))
	{
		forced = (std::sscanf(e, "%f %f", &forcedFocus, &forcedAperture) == 2);
		if (forced && LinearWorkflow())
			std::cout << "Depth of field: KINJO_DOF focus " << forcedFocus << ", aperture " << forcedAperture << std::endl;
	}
}

void SetDepthOfField(float focus, float aperture, float fadeSeconds)
{
	target.clear();
	StartFade(focus, aperture, fadeSeconds);
}

void SetDepthOfFieldTarget(const std::string& character, float aperture, float fadeSeconds)
{
	target = character;
	StartFade(CurrentFocus(), aperture, fadeSeconds);
}

const std::string& DepthOfFieldTarget()
{
	return target;
}

void SetDepthOfFieldFocus(float focus)
{
	// Following a target: the focus tracks it; a fade in progress still
	// eases from where the focus was.
	toFocus = std::max(focus, 1.0f);
	if (seconds <= 0.0f || Progress() >= 1.0f)
		fromFocus = toFocus;
}

void GetSceneDepthOfField(float& focus, float& aperture)
{
	focus = toFocus;
	aperture = toAperture;
}

bool DepthOfFieldActive()
{
	return LinearWorkflow() && allowed && !failed && CurrentAperture() > 0.01f;
}

void ApplyDepthOfField(TextureHandle source, TextureHandle depth, float nearPlane, float farPlane, int w, int h)
{
	produced = false;
	if (!DepthOfFieldActive() || !source || !depth || w <= 0 || h <= 0 || !EnsureShaders())
		return;
	EnsureTargets(w, h);

	const float scale = (float)h / 720.0f;   // apertures are given in 720p pixels
	const float maxBlur = kMaxBlur720 * scale;
	const float aperture = std::min(CurrentAperture(), kMaxBlur720) * scale;
	const glm::vec4 planes(nearPlane, farPlane, 0.0f, 0.0f);
	const float focus = CurrentFocus();
	const int hw = std::max(w / 2, 1), hh = std::max(h / 2, 1);

	RenderDevice& device = Device();
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Off;
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	// 1. Half resolution colour + blur size.
	device.BindFramebuffer(halfColorFbo);
	device.SetViewport(0, 0, hw, hh);
	prepare->UseShader();
	device.BindTexture(0, source);
	device.BindTexture(1, depth);
	device.SetUniform(prepareLocs.color, 0);
	device.SetUniform(prepareLocs.depth, 1);
	device.SetUniform(prepareLocs.lens, glm::vec4(focus, aperture * 0.5f, maxBlur * 0.5f, 0.0f));
	device.SetUniform(prepareLocs.planes, planes);
	device.SetUniform(prepareLocs.size, glm::vec2((float)hw, (float)hh));
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);

	// 2. The disc blur.
	device.BindFramebuffer(halfBlurFbo);
	blur->UseShader();
	device.BindTexture(0, halfColor);
	device.SetUniform(blurLocs.blurTex, 0);
	device.SetUniform(blurLocs.size, glm::vec2((float)hw, (float)hh));
	device.SetUniform(blurLocs.maxBlur, maxBlur * 0.5f);
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);

	// 3. Back to full resolution.
	device.BindFramebuffer(outputFbo);
	device.SetViewport(0, 0, w, h);
	composite->UseShader();
	device.BindTexture(0, source);
	device.BindTexture(1, depth);
	device.BindTexture(2, halfBlur);
	device.SetUniform(compositeLocs.color, 0);
	device.SetUniform(compositeLocs.depth, 1);
	device.SetUniform(compositeLocs.blurTex, 2);
	device.SetUniform(compositeLocs.lens, glm::vec4(focus, aperture, maxBlur, 0.0f));
	device.SetUniform(compositeLocs.planes, planes);
	device.SetUniform(compositeLocs.size, glm::vec2((float)w, (float)h));
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);

	device.BindFramebuffer(FramebufferHandle());
	produced = true;
}

TextureHandle DepthOfFieldOutput()
{
	return produced ? output : TextureHandle();
}

void ReleaseDepthOfField()
{
	ReleaseTargets();
	if (emptyVao)
		Device().DestroyVertexArray(emptyVao);
	delete prepare;
	delete blur;
	delete composite;
	prepare = blur = composite = nullptr;
	failed = false;
	produced = false;
}
