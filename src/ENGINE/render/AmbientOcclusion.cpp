// Screen-space ambient occlusion - see AmbientOcclusion.h. Backend-agnostic:
// GPU work goes through RenderDevice, shaders through ShaderProgram.

#include "AmbientOcclusion.h"
#include "RenderDevice.h"
#include "ColorPipeline.h"
#include "ProgramEvents.h"
#include "Reflections.h"
#include "../Shader.h"
#include "../RenderState.h"
#include "../UniformBlocks.h"
#include "../UniformBufferCache.h"
#include "../globals.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

namespace
{
	// --- settings -----------------------------------------------------------
	float projectStrength = 1.0f;    // renderer.dat `ao`
	float projectRadius = 60.0f;     // renderer.dat `aoRadius`
	int forced = -1;                 // KINJO_AO: 0 = off, 1 = on, -1 = not set
	float forcedRadius = -1.0f;      // KINJO_AO_RADIUS
	bool debugView = false;          // KINJO_AO_DEBUG
	float sceneStrength = -1.0f;     // < 0 = project default
	float sceneRadius = -1.0f;

	// Tuning (XeGTAO's defaults where they apply).
	const float kFalloff = 0.6f;          // occluders fade out over the outer 60% of the radius
	const float kPower = 2.2f;            // visibility^power: XeGTAO's fit to ray-traced references
	const float kMaxRadiusFraction = 0.2f;   // of the target height: caps the search when very close
	const float kDenoiseTolerance = 0.015f;  // plane distance, as a fraction of the view depth

	float EffectiveStrength()
	{
		if (forced == 0)
			return 0.0f;
		float v = (sceneStrength >= 0.0f) ? sceneStrength : projectStrength;
		return (forced == 1 && v <= 0.0f) ? 1.0f : std::min(v, 1.0f);
	}

	float EffectiveRadius()
	{
		if (forcedRadius > 0.0f)
			return forcedRadius;
		return (sceneRadius > 0.0f) ? sceneRadius : projectRadius;
	}

	// --- state --------------------------------------------------------------
	bool prepassDone = false;   // this frame's prepass is in the targets
	RenderState stateBeforePrepass;   // restored by EndAoPrepass
	bool rawDone = false;       // ...and the noisy occlusion from it
	bool active = false;        // ...and the denoised map lit shaders read

	// --- resources ----------------------------------------------------------
	int width = 0, height = 0;
	TextureHandle viewDepth;    // R32F linear view depth; 0 where nothing was drawn
	TextureHandle viewNormal;   // RGBA8 view-space normal * 0.5 + 0.5
	TextureHandle zBuffer;      // the prepass's own depth test
	TextureHandle rawAo;        // R8, noisy (one 4x4 tile of directions per pixel block)
	TextureHandle aoMap;        // R8, denoised: what lit shaders read
	FramebufferHandle prepassFbo, rawFbo, aoFbo;
	VertexArrayHandle emptyVao;

	ShaderProgram* prepass = nullptr;
	ShaderProgram* prepassInstanced = nullptr;
	ShaderProgram* gtao = nullptr;
	ShaderProgram* denoise = nullptr;
	bool shadersFailed = false;

	struct GtaoLocs { int proj = -1, size = -1, radius = -1, pixelsPerUnit = -1, maxRadiusPx = -1,
		falloff = -1, power = -1, depth = -1, normal = -1; };
	struct DenoiseLocs { int proj = -1, size = -1, tolerance = -1, raw = -1, depth = -1, normal = -1; };
	GtaoLocs gtaoLocs;
	DenoiseLocs denoiseLocs;
	glm::vec4 lastProj = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);   // for the denoise plane test

	// std140 mirror of the GLSL "AmbientOcclusion" block (shaders/ao.glsl).
	struct AoBlockData
	{
		int on;
		float strength;
		int debug;
		int reflectionsOn;   // screen-space reflections this frame (render/Reflections.h)
	};
	static_assert(sizeof(AoBlockData) == 16, "AoBlockData must match shaders/ao.glsl");
	const UniformBlockMember kAoMembers[] = {
		{ "aoOn", offsetof(AoBlockData, on), false },
		{ "aoStrength", offsetof(AoBlockData, strength), false },
		{ "aoDebug", offsetof(AoBlockData, debug), false },
		{ "ssrOn", offsetof(AoBlockData, reflectionsOn), false },
	};
	UniformBufferCache aoBlocks(UniformBlock::AmbientOcclusion, sizeof(AoBlockData), 2);

	struct ProgramLocs { bool hasBlock = false; int map = -1, depth = -1, reflection = -1; };
	std::unordered_map<unsigned int, ProgramLocs> locsByProgram;

	ShaderProgram* Build(const char* vert, const char* frag, const char* name)
	{
		ShaderProgram* s = new ShaderProgram(-1, vert, frag);
		s->SetNameString(name);
		if (s->GetID() == 0)
		{
			delete s;
			return nullptr;
		}
		return s;
	}

	void FreeShaders()
	{
		for (ShaderProgram** s : { &prepass, &prepassInstanced, &gtao, &denoise })
		{
			delete *s;
			*s = nullptr;
		}
	}

	bool EnsureShaders()
	{
		if (shadersFailed)
			return false;
		if (prepass != nullptr)
			return true;
		prepass = Build("data/shaders/ao_prepass.vert", "data/shaders/ao_prepass.frag", "ao_prepass");
		prepassInstanced = Build("data/shaders/ao_prepass_instanced.vert", "data/shaders/ao_prepass.frag",
			"ao_prepass_instanced");
		gtao = Build("data/shaders/resolve.vert", "data/shaders/gtao.frag", "gtao");
		denoise = Build("data/shaders/resolve.vert", "data/shaders/ao_denoise.frag", "ao_denoise");
		if (prepass == nullptr || prepassInstanced == nullptr || gtao == nullptr || denoise == nullptr)
		{
			std::cout << "ERROR: ambient occlusion shaders failed to build; ambient occlusion disabled" << std::endl;
			FreeShaders();
			shadersFailed = true;
			return false;
		}

		RenderDevice& device = Device();
		auto loc = [](const ShaderProgram* s, const char* n) { return ShaderProgram::DrawUniformLocation(s->GetID(), n); };
		auto sampler = [&](const ShaderProgram* s, const char* n) { return device.UniformLocation(ProgramHandle(s->GetID()), n); };
		gtaoLocs.proj = loc(gtao, "proj");
		gtaoLocs.size = loc(gtao, "size");
		gtaoLocs.radius = loc(gtao, "radius");
		gtaoLocs.pixelsPerUnit = loc(gtao, "pixelsPerUnit");
		gtaoLocs.maxRadiusPx = loc(gtao, "maxRadiusPx");
		gtaoLocs.falloff = loc(gtao, "falloff");
		gtaoLocs.power = loc(gtao, "power");
		gtaoLocs.depth = sampler(gtao, "aoViewDepth");
		gtaoLocs.normal = sampler(gtao, "aoViewNormal");
		denoiseLocs.proj = loc(denoise, "proj");
		denoiseLocs.size = loc(denoise, "size");
		denoiseLocs.tolerance = loc(denoise, "tolerance");
		denoiseLocs.raw = sampler(denoise, "aoRaw");
		denoiseLocs.depth = sampler(denoise, "aoViewDepth");
		denoiseLocs.normal = sampler(denoise, "aoViewNormal");
		return true;
	}

	void ReleaseTargets()
	{
		RenderDevice& device = Device();
		for (TextureHandle* t : { &viewDepth, &viewNormal, &zBuffer, &rawAo, &aoMap })
			if (*t)
				device.DestroyTexture(*t);
		for (FramebufferHandle* f : { &prepassFbo, &rawFbo, &aoFbo })
			if (*f)
				device.DestroyFramebuffer(*f);
		width = height = 0;
	}

	bool EnsureTargets(int w, int h)
	{
		if (viewDepth && w == width && h == height)
			return true;
		ReleaseTargets();
		RenderDevice& device = Device();
		auto make = [&](TextureFormat format)
		{
			TextureDesc desc;
			desc.format = format;
			desc.width = w;
			desc.height = h;
			desc.filter = TextureFilter::Nearest;   // read with texelFetch only
			desc.wrap = TextureWrap::ClampToEdge;
			return device.CreateTexture(desc);
		};
		viewDepth = make(TextureFormat::R32F);
		viewNormal = make(TextureFormat::RGBA8);
		zBuffer = make(TextureFormat::Depth24);
		rawAo = make(TextureFormat::R8);
		aoMap = make(TextureFormat::R8);
		width = w;
		height = h;

		prepassFbo = device.CreateFramebuffer();
		device.AttachTexture(prepassFbo, Attachment::Color0, viewDepth);
		device.AttachTexture(prepassFbo, Attachment::Color1, viewNormal);
		device.AttachTexture(prepassFbo, Attachment::Depth, zBuffer);
		device.SetDrawBuffers(prepassFbo, 2);
		rawFbo = device.CreateFramebuffer();
		device.AttachTexture(rawFbo, Attachment::Color0, rawAo);
		aoFbo = device.CreateFramebuffer();
		device.AttachTexture(aoFbo, Attachment::Color0, aoMap);
		if (!emptyVao)
			emptyVao = device.CreateVertexArray();

		bool ok = true;
		for (FramebufferHandle f : { prepassFbo, rawFbo, aoFbo })
		{
			std::string error;
			if (!device.IsFramebufferComplete(f, &error))
			{
				std::cout << "ERROR: ambient occlusion target incomplete (" << error << "); ambient occlusion disabled" << std::endl;
				ok = false;
			}
		}
		device.BindFramebuffer(FramebufferHandle());
		if (!ok)
		{
			ReleaseTargets();
			shadersFailed = true;   // don't retry every frame
		}
		return ok;
	}

	// Full-screen data passes: no blending (they write values, not colour),
	// no depth.
	RenderState DataPassState()
	{
		RenderState s = CurrentRenderState();
		s.blend = BlendMode::Off;
		s.depthTest = false;
		s.depthWrite = false;
		s.cull = CullMode::None;
		return s;
	}

	glm::vec4 ProjectionInfo(const glm::mat4& p)
	{
		return glm::vec4(p[0][0], p[1][1], p[2][0], p[2][1]);
	}
}

void LoadAmbientOcclusionSettings()
{
	auto config = GetMapStringsFromFile(RendererConfigPath());
	auto parse = [](const std::string& s, float fallback) -> float
	{
		try { return std::max(std::stof(s), 0.0f); }
		catch (...) { return fallback; }
	};
	projectStrength = (config.count("ao") > 0) ? std::min(parse(config["ao"], 1.0f), 1.0f) : 1.0f;
	projectRadius = (config.count("aoRadius") > 0) ? parse(config["aoRadius"], 60.0f) : 60.0f;
	if (projectRadius <= 0.0f)
		projectRadius = 60.0f;
	if (const char* e = std::getenv("KINJO_AO"))
		forced = (e[0] == '0') ? 0 : (e[0] == '1') ? 1 : -1;
	if (const char* r = std::getenv("KINJO_AO_RADIUS"))
		forcedRadius = parse(r, -1.0f);
	if (const char* d = std::getenv("KINJO_AO_DEBUG"))
		debugView = (d[0] == '1');
	if (LinearWorkflow())
		std::cout << "Ambient occlusion: strength " << projectStrength << ", radius " << projectRadius
			<< (forced == 0 ? " (KINJO_AO=0: off)" : forced == 1 ? " (KINJO_AO=1: forced on)" : "")
			<< (forcedRadius > 0.0f ? " (KINJO_AO_RADIUS overrides scenes)" : "")
			<< (debugView ? " (KINJO_AO_DEBUG: showing the occlusion)" : "") << std::endl;
}

bool AmbientOcclusionWanted()
{
	return LinearWorkflow() && !shadersFailed && EffectiveStrength() > 0.0f;
}

bool PrepassWanted()
{
	return LinearWorkflow() && !shadersFailed && (EffectiveStrength() > 0.0f || ReflectionsWanted());
}

TextureHandle PrepassViewDepth()
{
	return prepassDone ? viewDepth : TextureHandle();
}

TextureHandle PrepassViewNormal()
{
	return prepassDone ? viewNormal : TextureHandle();
}

void BeginAoFrame()
{
	prepassDone = rawDone = active = false;
	BindAmbientOcclusion(0);   // the block always holds a valid "off" until a map exists
}

bool BeginAoPrepass(int w, int h, unsigned int& program, unsigned int& instancedProgram)
{
	program = instancedProgram = 0;
	if (!PrepassWanted() || w <= 0 || h <= 0 || !EnsureShaders() || !EnsureTargets(w, h))
		return false;

	RenderDevice& device = Device();
	device.BindFramebuffer(prepassFbo);
	device.SetViewport(0, 0, width, height);
	stateBeforePrepass = CurrentRenderState();
	RenderState s = stateBeforePrepass;
	s.blend = BlendMode::Off;   // MRT data: a blend would mix depths
	s.depthTest = true;
	s.depthWrite = true;
	s.depthCompare = CompareOp::Less;
	s.cull = CullMode::None;    // as the world pass draws models
	s.depthBiasFactor = s.depthBiasUnits = 0.0f;
	ApplyRenderState(s);
	device.Clear(true, true, glm::vec4(0.0f));   // depth 0 = nothing drawn here

	program = prepass->GetID();
	instancedProgram = prepassInstanced->GetID();
	return true;
}

void EndAoPrepass(int screenWidth, int screenHeight)
{
	RenderDevice& device = Device();
	ApplyRenderState(stateBeforePrepass);
	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, screenWidth, screenHeight);
	prepassDone = true;
}

void ComputeAmbientOcclusion(const glm::mat4& projection)
{
	if (!prepassDone || gtao == nullptr)
		return;
	RenderDevice& device = Device();
	ScopedRenderState scope(DataPassState());

	const float radius = EffectiveRadius();
	lastProj = ProjectionInfo(projection);

	device.BindFramebuffer(rawFbo);
	device.SetViewport(0, 0, width, height);
	gtao->UseShader();
	device.BindTexture(0, viewDepth);
	device.BindTexture(1, viewNormal);
	device.SetUniform(gtaoLocs.depth, 0);
	device.SetUniform(gtaoLocs.normal, 1);
	device.SetUniform(gtaoLocs.proj, lastProj);
	device.SetUniform(gtaoLocs.size, glm::vec2((float)width, (float)height));
	device.SetUniform(gtaoLocs.radius, radius);
	// Pixels per world unit at view depth 1: half the target width times the
	// projection's x scale.
	device.SetUniform(gtaoLocs.pixelsPerUnit, 0.5f * (float)width * std::abs(lastProj.x));
	device.SetUniform(gtaoLocs.maxRadiusPx, kMaxRadiusFraction * (float)height);
	device.SetUniform(gtaoLocs.falloff, kFalloff);
	device.SetUniform(gtaoLocs.power, kPower);
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	rawDone = true;
}

void DenoiseAmbientOcclusion(int screenWidth, int screenHeight)
{
	if (!rawDone || denoise == nullptr)
		return;
	RenderDevice& device = Device();
	{
		ScopedRenderState scope(DataPassState());
		device.BindFramebuffer(aoFbo);
		device.SetViewport(0, 0, width, height);
		denoise->UseShader();
		device.BindTexture(0, rawAo);
		device.BindTexture(1, viewDepth);
		device.BindTexture(2, viewNormal);
		device.SetUniform(denoiseLocs.raw, 0);
		device.SetUniform(denoiseLocs.depth, 1);
		device.SetUniform(denoiseLocs.normal, 2);
		device.SetUniform(denoiseLocs.proj, lastProj);
		device.SetUniform(denoiseLocs.size, glm::vec2((float)width, (float)height));
		device.SetUniform(denoiseLocs.tolerance, kDenoiseTolerance);
		device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	}
	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, screenWidth, screenHeight);
	active = true;
}

bool AmbientOcclusionActive()
{
	return active;
}

void BindAmbientOcclusion(unsigned int program)
{
	AoBlockData data = {};
	data.on = active ? 1 : 0;
	data.strength = EffectiveStrength();
	data.debug = debugView ? 1 : 0;
	data.reflectionsOn = ReflectionsActive() ? 1 : 0;
	aoBlocks.Bind(&data);
	if (program == 0)
		return;

	RenderDevice& device = Device();
	auto it = locsByProgram.find(program);
	if (it == locsByProgram.end())
	{
		// Forget a program's entry when it is deleted (GL reuses ids).
		AddProgramDeletedListener([](unsigned int p) { locsByProgram.erase(p); });
		ProgramLocs l;
		l.hasBlock = device.HasUniformBlock(ProgramHandle(program), "AmbientOcclusion");
		l.map = device.UniformLocation(ProgramHandle(program), "aoMap");
		l.depth = device.UniformLocation(ProgramHandle(program), "aoDepth");
		l.reflection = device.UniformLocation(ProgramHandle(program), "ssrMap");
		if (l.hasBlock)
			CheckUniformBlockLayout(program, "AmbientOcclusion", kAoMembers,
				sizeof(kAoMembers) / sizeof(kAoMembers[0]), sizeof(AoBlockData));
		it = locsByProgram.emplace(program, l).first;
	}
	if (!it->second.hasBlock)
		return;
	if (prepassDone)
	{
		// The prepass depth: both occlusion and reflections check fragments against it.
		device.BindTexture(13, viewDepth);
		device.SetUniform(it->second.depth, 13);
	}
	if (active)
	{
		device.BindTexture(12, aoMap);
		device.SetUniform(it->second.map, 12);
	}
	if (data.reflectionsOn)
	{
		device.BindTexture(15, ReflectionTexture());
		device.SetUniform(it->second.reflection, 15);
	}
}

void SetSceneAO(float strength, float radius)
{
	sceneStrength = (strength < 0.0f) ? -1.0f : std::min(strength, 1.0f);
	sceneRadius = (radius <= 0.0f) ? -1.0f : radius;
}

void GetProjectAO(float& strength, float& radius)
{
	strength = projectStrength;
	radius = projectRadius;
}

void SetAmbientOcclusionDebugView(bool on)
{
	debugView = on;
}

bool AmbientOcclusionDebugView()
{
	return debugView;
}

void GetSceneAO(float& strength, float& radius)
{
	strength = sceneStrength;
	radius = sceneRadius;
}

void ReleaseAmbientOcclusion()
{
	ReleaseTargets();
	if (emptyVao)
		Device().DestroyVertexArray(emptyVao);
	FreeShaders();
	locsByProgram.clear();
	prepassDone = rawDone = active = false;
	shadersFailed = false;
}
