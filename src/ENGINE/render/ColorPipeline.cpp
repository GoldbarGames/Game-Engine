// The linear workflow - see ColorPipeline.h. Backend-agnostic: GPU work goes
// through RenderDevice, shaders through ShaderProgram.

#include "ColorPipeline.h"
#include "RenderDevice.h"
#include "TemporalAA.h"
#include "ColorGrading.h"
#include "../FrameBuffer.h"
#include "../Shader.h"
#include "../Texture.h"
#include "../RenderState.h"
#include "../UniformBlocks.h"
#include "../UniformBufferCache.h"
#include "../globals.h"
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <set>
#include <vector>

namespace
{
	bool linearOn = false;

	// std140 mirror of the GLSL "KinjoTarget" block (shaders/target.glsl).
	struct TargetBlockData
	{
		int linear;
		int pad[3];
	};
	UniformBufferCache targetBlocks(UniformBlock::Target, sizeof(TargetBlockData), 2);

	// The HDR world colour, shared by every world target: it is resolved
	// straight after each world render, so the main world and the crossfade
	// capture can reuse it. Each main framebuffer gets its own framebuffer
	// object pairing it with that framebuffer's depth and character mask.
	TextureHandle hdrColor;
	// Screen-space motion of the scene's objects (temporal anti-aliasing only):
	// colour attachment 2 of every world target. Cleared with the world; lit
	// Scene3D shaders write it (shaders/motion.glsl).
	TextureHandle hdrMotion;
	int hdrWidth = 0;
	int hdrHeight = 0;

	struct WorldFbo
	{
		unsigned int depth;
		unsigned int mask;
		FramebufferHandle fbo;
	};
	std::vector<WorldFbo> worldFbos;
	FramebufferHandle colorOnlyFbo;   // hdrColor alone: drawing over the world while sampling its depth
	// What Bloom and Resolve read instead of hdrColor, while set (the temporal
	// anti-aliasing output for the main world).
	TextureHandle sourceOverride;

	TextureHandle WorldSource()
	{
		return sourceOverride ? sourceOverride : hdrColor;
	}

	// Exposure and tonemapping (item 2)
	Tonemapper tonemapper = Tonemapper::AgXPunchy;
	float projectExposure = 1.0f;     // renderer.dat `exposure`
	float forcedExposure = 0.0f;      // KINJO_EXPOSURE (> 0 overrides everything)
	float sceneExposure = 0.0f;       // the scene's value; 0 = project default
	// A fade in progress (scene3d exposure <v> <seconds>)
	bool fading = false;
	float fadeFrom = 1.0f;
	float fadeTo = 1.0f;
	float fadeSeconds = 0.0f;
	std::chrono::steady_clock::time_point fadeStart;

	// Resolve: one full-screen triangle (no vertex data) that exposes,
	// tonemaps and encodes the HDR world - shaders/resolve.vert/.frag. If those
	// can't be built, this embedded encode-only version keeps the world visible.
	const char* RESOLVE_VERT =
		"#version 300 es\n"
		"precision highp float;\n"
		"void main()\n"
		"{\n"
		"    vec2 p = vec2(gl_VertexID == 1 ? 3.0 : -1.0, gl_VertexID == 2 ? 3.0 : -1.0);\n"
		"    gl_Position = vec4(p, 0.0, 1.0);\n"
		"}\n";

	const char* RESOLVE_FRAG =
		"#version 300 es\n"
		"precision highp float;\n"
		"#define KINJO_LINEAR_OUTPUT\n"   // writes the gamma target itself
		"uniform sampler2D hdrColor;\n"
		"out vec4 color;\n"
		"vec3 LinearToSrgb(vec3 c)\n"
		"{\n"
		"    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));\n"
		"}\n"
		"void main()\n"
		"{\n"
		"    vec4 h = texelFetch(hdrColor, ivec2(gl_FragCoord.xy), 0);\n"
		"    color = vec4(LinearToSrgb(clamp(h.rgb, 0.0, 1.0)), clamp(h.a, 0.0, 1.0));\n"
		"}\n";

	ShaderProgram* resolveShader = nullptr;
	bool resolveFailed = false;
	int resolveTextureLoc = -1;
	int resolveExposureLoc = -1;
	int resolveTonemapLoc = -1;
	int resolveBloomTexLoc = -1;
	int resolveBloomStrengthLoc = -1;
	int resolveBloomLevelsLoc = -1;
	struct GradeLocs { int lut = -1, prevLut = -1, size = -1, strength = -1, prevSize = -1, prevStrength = -1, blend = -1; };
	GradeLocs gradeLocs;
	VertexArrayHandle emptyVao;   // attribute-less draws still need a VAO bound (core profile)

	VertexArrayHandle EmptyVao()
	{
		if (!emptyVao)
			emptyVao = Device().CreateVertexArray();
		return emptyVao;
	}

	// Bloom (item 3): half resolution down to ~1/64, RGBA16F, linear filtered.
	float projectBloom = 0.04f;    // renderer.dat `bloom`
	float forcedBloom = -1.0f;     // KINJO_BLOOM (>= 0 overrides everything)
	float sceneBloom = -1.0f;      // the scene's value; < 0 = project default
	const int kMaxBloomLevels = 6;
	struct BloomLevel
	{
		TextureHandle texture;
		FramebufferHandle fbo;
		int width = 0;
		int height = 0;
	};
	BloomLevel bloomChain[kMaxBloomLevels];
	int bloomLevelCount = 0;
	bool bloomReady = false;       // the chain holds this frame's world (Resolve uses it once)

	ShaderProgram* bloomDown = nullptr;
	ShaderProgram* bloomUp = nullptr;
	bool bloomFailed = false;
	struct BloomLocs { int texture = -1, srcTexel = -1, dstSize = -1, karis = -1; };
	BloomLocs downLocs, upLocs;

	BloomLocs FindBloomLocs(const ShaderProgram& s)
	{
		BloomLocs l;
		const unsigned int id = s.GetID();
		l.texture = Device().UniformLocation(ProgramHandle(id), "srcTexture");
		l.srcTexel = ShaderProgram::DrawUniformLocation(id, "srcTexel");
		l.dstSize = ShaderProgram::DrawUniformLocation(id, "dstSize");
		l.karis = ShaderProgram::DrawUniformLocation(id, "karis");
		return l;
	}

	bool EnsureBloomShaders()
	{
		if (bloomFailed)
			return false;
		if (bloomDown != nullptr)
			return true;
		bloomDown = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/bloom_down.frag");
		bloomDown->SetNameString("bloom_down");
		bloomUp = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/bloom_up.frag");
		bloomUp->SetNameString("bloom_up");
		if (bloomDown->GetID() == 0 || bloomUp->GetID() == 0)
		{
			std::cout << "ERROR: bloom shaders failed to build; bloom disabled" << std::endl;
			delete bloomDown;
			delete bloomUp;
			bloomDown = bloomUp = nullptr;
			bloomFailed = true;
			return false;
		}
		downLocs = FindBloomLocs(*bloomDown);
		upLocs = FindBloomLocs(*bloomUp);
		return true;
	}

	void ReleaseBloomChain()
	{
		RenderDevice& device = Device();
		for (BloomLevel& l : bloomChain)
		{
			if (l.fbo)
				device.DestroyFramebuffer(l.fbo);
			if (l.texture)
				device.DestroyTexture(l.texture);
			l.width = l.height = 0;
		}
		bloomLevelCount = 0;
		bloomReady = false;
	}

	// Sized from the HDR world: half, quarter, ... while both sides stay >= 8.
	void EnsureBloomChain(int width, int height)
	{
		if (bloomLevelCount > 0 && bloomChain[0].width == std::max(1, width / 2)
			&& bloomChain[0].height == std::max(1, height / 2))
			return;
		ReleaseBloomChain();
		RenderDevice& device = Device();
		int w = width / 2, h = height / 2;
		while (bloomLevelCount < kMaxBloomLevels && w >= 8 && h >= 8)
		{
			BloomLevel& l = bloomChain[bloomLevelCount];
			TextureDesc desc;
			desc.format = TextureFormat::RGBA16F;
			desc.width = w;
			desc.height = h;
			desc.filter = TextureFilter::Linear;
			desc.wrap = TextureWrap::ClampToEdge;
			l.texture = device.CreateTexture(desc);
			l.fbo = device.CreateFramebuffer();
			device.AttachTexture(l.fbo, Attachment::Color0, l.texture);
			l.width = w;
			l.height = h;
			bloomLevelCount++;
			w /= 2;
			h /= 2;
		}
		device.BindFramebuffer(FramebufferHandle());
	}

	bool EnsureResolve()
	{
		if (resolveFailed)
			return false;
		if (resolveShader != nullptr)
			return true;

		resolveShader = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/resolve.frag");
		resolveShader->SetNameString("resolve");
		if (resolveShader->GetID() == 0)
		{
			std::cout << "ERROR: shaders/resolve.* failed to build; the linear world is encoded "
				"without exposure or tonemapping" << std::endl;
			delete resolveShader;
			const std::string vert = ShaderProgram::ApplyVersion(RESOLVE_VERT);
			const std::string frag = ShaderProgram::ApplyVersion(RESOLVE_FRAG);
			resolveShader = new ShaderProgram(-1, vert.c_str(), frag.c_str(), true);
			resolveShader->SetNameString("resolve_fallback");
		}
		if (resolveShader->GetID() == 0)
		{
			std::cout << "ERROR: linear workflow resolve shader failed to build; the world will look wrong" << std::endl;
			delete resolveShader;
			resolveShader = nullptr;
			resolveFailed = true;
			return false;
		}
		const unsigned int id = resolveShader->GetID();
		resolveTextureLoc = Device().UniformLocation(ProgramHandle(id), "hdrColor");
		resolveExposureLoc = ShaderProgram::DrawUniformLocation(id, "exposure");
		resolveTonemapLoc = ShaderProgram::DrawUniformLocation(id, "tonemap");
		resolveBloomTexLoc = Device().UniformLocation(ProgramHandle(id), "bloomTexture");
		resolveBloomStrengthLoc = ShaderProgram::DrawUniformLocation(id, "bloomStrength");
		resolveBloomLevelsLoc = ShaderProgram::DrawUniformLocation(id, "bloomLevels");
		gradeLocs.lut = Device().UniformLocation(ProgramHandle(id), "gradeLut");
		gradeLocs.prevLut = Device().UniformLocation(ProgramHandle(id), "prevGradeLut");
		gradeLocs.size = ShaderProgram::DrawUniformLocation(id, "gradeSize");
		gradeLocs.strength = ShaderProgram::DrawUniformLocation(id, "gradeStrength");
		gradeLocs.prevSize = ShaderProgram::DrawUniformLocation(id, "prevGradeSize");
		gradeLocs.prevStrength = ShaderProgram::DrawUniformLocation(id, "prevGradeStrength");
		gradeLocs.blend = ShaderProgram::DrawUniformLocation(id, "gradeBlend");
		return true;
	}

	bool ParseTonemapper(const std::string& name, Tonemapper& out)
	{
		if (name == "none") { out = Tonemapper::None; return true; }
		if (name == "agx") { out = Tonemapper::AgX; return true; }
		if (name == "agx_punchy") { out = Tonemapper::AgXPunchy; return true; }
		if (name == "aces") { out = Tonemapper::ACES; return true; }
		return false;
	}

	float ParsePositive(const std::string& s, float fallback)
	{
		try
		{
			const float v = std::stof(s);
			return (v > 0.0f) ? v : fallback;
		}
		catch (...)
		{
			return fallback;
		}
	}

	// Comments out of the way before looking for declarations (a commented-out
	// `out vec4 old;` must not be mistaken for the real output).
	std::string StripComments(const std::string& s)
	{
		std::string out;
		out.reserve(s.size());
		for (size_t i = 0; i < s.size(); i++)
		{
			if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '/')
			{
				while (i < s.size() && s[i] != '\n') i++;
				out += '\n';
			}
			else if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '*')
			{
				i += 2;
				while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/')) i++;
				i++;
				out += ' ';
			}
			else
			{
				out += s[i];
			}
		}
		return out;
	}
}

const std::string& RendererConfigPath()
{
	static const std::string path = []()
	{
		const char* e = std::getenv("KINJO_RENDERER_DAT");
		return std::string((e != nullptr && e[0] != '\0') ? e : "data/config/renderer.dat");
	}();
	return path;
}

void ReloadColorSettings()
{
	const bool mode = linearOn;
	LoadColorSettings();
	linearOn = mode;
}

void LoadColorSettings()
{
	auto config = GetMapStringsFromFile(RendererConfigPath());

	bool want = false;
	const char* env = std::getenv("KINJO_LINEAR");
	if (env != nullptr && (env[0] == '0' || env[0] == '1'))
		want = (env[0] == '1');
	else
		want = (config.count("linearLighting") > 0 && config["linearLighting"] == "1");

	// Tonemapper and exposure (renderer.dat, then the one-run overrides).
	tonemapper = Tonemapper::AgXPunchy;
	if (config.count("tonemap") > 0 && !ParseTonemapper(config["tonemap"], tonemapper))
		std::cout << "WARNING: renderer.dat tonemap '" << config["tonemap"]
			<< "' unknown (none, agx, agx_punchy, aces) - using agx_punchy" << std::endl;
	if (const char* t = std::getenv("KINJO_TONEMAP"))
		ParseTonemapper(t, tonemapper);
	projectExposure = (config.count("exposure") > 0) ? ParsePositive(config["exposure"], 1.0f) : 1.0f;
	if (const char* e = std::getenv("KINJO_EXPOSURE"))
		forcedExposure = ParsePositive(e, 0.0f);

	// Bloom strength: 0..1 (0 = off).
	auto parseStrength = [](const std::string& s, float fallback) -> float
	{
		try { return std::min(std::max(std::stof(s), 0.0f), 1.0f); }
		catch (...) { return fallback; }
	};
	projectBloom = (config.count("bloom") > 0) ? parseStrength(config["bloom"], 0.04f) : 0.04f;
	if (const char* b = std::getenv("KINJO_BLOOM"))
		forcedBloom = parseStrength(b, -1.0f);

	if (want && !Device().SupportsFloatRenderTargets())
	{
		std::cout << "WARNING: linearLighting needs float render targets, which this "
			"platform lacks - staying with the gamma pipeline" << std::endl;
		want = false;
	}

	linearOn = want;
	if (linearOn)
	{
		std::cout << "Colour pipeline: LINEAR (sRGB scene textures, RGBA16F world), tonemap "
			<< TonemapperName(tonemapper) << ", exposure " << projectExposure << ", bloom " << projectBloom;
		if (forcedExposure > 0.0f)
			std::cout << " (KINJO_EXPOSURE " << forcedExposure << " overrides scenes)";
		if (forcedBloom >= 0.0f)
			std::cout << " (KINJO_BLOOM " << forcedBloom << " overrides scenes)";
		std::cout << std::endl;
	}
}

Tonemapper ActiveTonemapper()
{
	return tonemapper;
}

const char* TonemapperName(Tonemapper t)
{
	switch (t)
	{
	case Tonemapper::None:      return "none";
	case Tonemapper::AgX:       return "agx";
	case Tonemapper::AgXPunchy: return "agx_punchy";
	case Tonemapper::ACES:      return "aces";
	default:                    return "?";
	}
}

void SetSceneExposure(float multiplier, float seconds)
{
	const float target = (multiplier > 0.0f) ? multiplier : 0.0f;
	if (seconds > 0.0f)
	{
		fadeFrom = EffectiveExposure();
		fadeTo = (target > 0.0f) ? target : projectExposure;
		fadeSeconds = seconds;
		fadeStart = std::chrono::steady_clock::now();
		fading = true;
	}
	else
	{
		fading = false;
	}
	sceneExposure = target;
}

float SceneExposure()
{
	return sceneExposure;
}

float EffectiveExposure()
{
	if (forcedExposure > 0.0f)
		return forcedExposure;
	if (fading)
	{
		const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - fadeStart).count() / fadeSeconds;
		if (t < 1.0f)
			return fadeFrom + (fadeTo - fadeFrom) * t;
		fading = false;
	}
	return (sceneExposure > 0.0f) ? sceneExposure : projectExposure;
}

bool LinearWorkflow()
{
	return linearOn;
}

glm::vec3 SrgbToLinear(const glm::vec3& c)
{
	glm::vec3 out;
	for (int i = 0; i < 3; i++)
	{
		const float v = c[i];
		out[i] = (v <= 0.04045f) ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
	}
	return out;
}

glm::vec3 SceneColor(const glm::vec3& srgb)
{
	return linearOn ? SrgbToLinear(glm::max(srgb, glm::vec3(0.0f))) : srgb;
}

glm::vec4 SceneColor(const glm::vec4& srgb)
{
	return linearOn ? glm::vec4(SceneColor(glm::vec3(srgb)), srgb.a) : srgb;
}

void SetTargetLinear(bool linear)
{
	TargetBlockData d = {};
	d.linear = linear ? 1 : 0;
	targetBlocks.Bind(&d);
}

// ------------------------------------------------------------ world target

void BindWorldTarget(FrameBuffer& fb)
{
	RenderDevice& device = Device();
	if (!linearOn)
	{
		device.BindFramebuffer(FramebufferHandle(fb.framebufferObject));
		return;
	}

	const int w = fb.sprite->texture->GetWidth();
	const int h = fb.sprite->texture->GetHeight();
	if (!hdrColor || w != hdrWidth || h != hdrHeight)
	{
		ReleaseWorldTargets();
		TextureDesc desc;
		desc.format = TextureFormat::RGBA16F;
		desc.width = w;
		desc.height = h;
		desc.filter = TextureFilter::Linear;   // bloom's first downsample filters it; Resolve uses texelFetch
		desc.wrap = TextureWrap::ClampToEdge;
		hdrColor = device.CreateTexture(desc);
		if (TemporalAAWanted())
		{
			desc.filter = TextureFilter::Nearest;   // read with texelFetch
			hdrMotion = device.CreateTexture(desc);
		}
		hdrWidth = w;
		hdrHeight = h;
	}

	FramebufferHandle fbo;
	for (const WorldFbo& t : worldFbos)
		if (t.depth == fb.depthTexture && t.mask == fb.maskTexture)
			fbo = t.fbo;
	if (!fbo)
	{
		fbo = device.CreateFramebuffer();
		device.AttachTexture(fbo, Attachment::Color0, hdrColor);
		device.AttachTexture(fbo, Attachment::Color1, TextureHandle(fb.maskTexture));
		if (hdrMotion)
			device.AttachTexture(fbo, Attachment::Color2, hdrMotion);
		device.AttachTexture(fbo, Attachment::DepthStencil, TextureHandle(fb.depthTexture));
		std::string error;
		if (!device.IsFramebufferComplete(fbo, &error))
			std::cout << "ERROR: linear world target incomplete (" << error << ")" << std::endl;
		worldFbos.push_back({ fb.depthTexture, fb.maskTexture, fbo });
	}

	device.BindFramebuffer(fbo);
	SetTargetLinear(true);
}

void ResolveWorldTarget(FrameBuffer& fb)
{
	if (!linearOn)
		return;
	SetTargetLinear(false);
	if (!hdrColor || !EnsureResolve())
		return;

	RenderDevice& device = Device();
	device.BindFramebuffer(FramebufferHandle(fb.framebufferObject));
	device.SetViewport(0, 0, hdrWidth, hdrHeight);

	// Overwrite colour only: the depth and mask stay as the world left them
	// (the toon outline reads both in the composite).
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Off;
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	resolveShader->UseShader();
	device.BindTexture(0, WorldSource());
	device.SetUniform(resolveTextureLoc, 0);
	device.SetUniform(resolveExposureLoc, EffectiveExposure());
	device.SetUniform(resolveTonemapLoc, (int)tonemapper);
	if (bloomReady && bloomLevelCount > 0)
	{
		device.BindTexture(1, bloomChain[0].texture);
		device.SetUniform(resolveBloomTexLoc, 1);
		device.SetUniform(resolveBloomStrengthLoc, EffectiveBloom());
		device.SetUniform(resolveBloomLevelsLoc, (float)bloomLevelCount);
	}
	else
	{
		device.SetUniform(resolveBloomStrengthLoc, 0.0f);
	}
	// Colour grading (render/ColorGrading.h): LUTs on units 2 and 3.
	ColorGradeState grade;
	if (CurrentColorGrade(grade))
	{
		device.BindTexture(2, grade.lut);
		device.BindTexture(3, grade.previous);
		device.SetUniform(gradeLocs.lut, 2);
		device.SetUniform(gradeLocs.prevLut, 3);
	}
	device.SetUniform(gradeLocs.size, grade.lut ? grade.lutSize : 0.0f);
	device.SetUniform(gradeLocs.strength, grade.strength);
	device.SetUniform(gradeLocs.prevSize, grade.previous ? grade.previousSize : 0.0f);
	device.SetUniform(gradeLocs.prevStrength, grade.previousStrength);
	device.SetUniform(gradeLocs.blend, grade.blend);
	device.Draw(EmptyVao(), Primitive::Triangles, 0, 3);
	bloomReady = false;   // the chain belongs to this world image only
}

void ReleaseWorldTargets()
{
	RenderDevice& device = Device();
	for (WorldFbo& t : worldFbos)
		device.DestroyFramebuffer(t.fbo);
	worldFbos.clear();
	if (colorOnlyFbo)
		device.DestroyFramebuffer(colorOnlyFbo);
	sourceOverride = TextureHandle();
	if (hdrColor)
		device.DestroyTexture(hdrColor);
	if (hdrMotion)
		device.DestroyTexture(hdrMotion);
	hdrWidth = hdrHeight = 0;
	ReleaseBloomChain();
}

TextureHandle WorldHdrTexture()
{
	return linearOn ? hdrColor : TextureHandle();
}

TextureHandle WorldMotionTexture()
{
	return linearOn ? hdrMotion : TextureHandle();
}

bool BindWorldColorOnly()
{
	if (!linearOn || !hdrColor)
		return false;
	RenderDevice& device = Device();
	if (!colorOnlyFbo)
	{
		colorOnlyFbo = device.CreateFramebuffer();
		device.AttachTexture(colorOnlyFbo, Attachment::Color0, hdrColor);
		std::string error;
		if (!device.IsFramebufferComplete(colorOnlyFbo, &error))
			std::cout << "ERROR: linear world colour target incomplete (" << error << ")" << std::endl;
	}
	device.BindFramebuffer(colorOnlyFbo);
	device.SetViewport(0, 0, hdrWidth, hdrHeight);
	SetTargetLinear(true);
	return true;
}

void SetWorldSourceOverride(TextureHandle texture)
{
	sourceOverride = texture;
}

void ReleaseColorPipeline()
{
	ReleaseWorldTargets();
	if (resolveShader != nullptr)
	{
		delete resolveShader;
		resolveShader = nullptr;
	}
	delete bloomDown;
	delete bloomUp;
	bloomDown = bloomUp = nullptr;
	if (emptyVao)
		Device().DestroyVertexArray(emptyVao);
	resolveFailed = false;
	bloomFailed = false;
}

// ------------------------------------------------------------------- bloom

void SetSceneBloom(float strength)
{
	sceneBloom = (strength < 0.0f) ? -1.0f : std::min(strength, 1.0f);
}

float SceneBloom()
{
	return sceneBloom;
}

float EffectiveBloom()
{
	if (forcedBloom >= 0.0f)
		return forcedBloom;
	return (sceneBloom >= 0.0f) ? sceneBloom : projectBloom;
}

bool BloomEnabled()
{
	return linearOn && EffectiveBloom() > 0.0f && !bloomFailed;
}

void BloomWorldTarget()
{
	bloomReady = false;
	if (!BloomEnabled() || !hdrColor || !EnsureBloomShaders())
		return;
	EnsureBloomChain(hdrWidth, hdrHeight);
	if (bloomLevelCount == 0)
		return;

	RenderDevice& device = Device();
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Off;
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	// Down: the world -> level 0 (Karis average) -> level 1 -> ...
	bloomDown->UseShader();
	device.SetUniform(downLocs.texture, 0);
	TextureHandle source = WorldSource();
	int sw = hdrWidth, sh = hdrHeight;
	for (int i = 0; i < bloomLevelCount; i++)
	{
		const BloomLevel& dst = bloomChain[i];
		device.BindFramebuffer(dst.fbo);
		device.SetViewport(0, 0, dst.width, dst.height);
		device.BindTexture(0, source);
		device.SetUniform(downLocs.srcTexel, glm::vec2(1.0f / sw, 1.0f / sh));
		device.SetUniform(downLocs.dstSize, glm::vec2((float)dst.width, (float)dst.height));
		device.SetUniform(downLocs.karis, (i == 0) ? 1 : 0);
		device.Draw(EmptyVao(), Primitive::Triangles, 0, 3);
		source = dst.texture;
		sw = dst.width;
		sh = dst.height;
	}

	// Up: each level's tent-filtered blur ADDED onto the next larger one, so
	// level 0 ends up with the sum of every level (Resolve divides it back).
	RenderState additive = s;
	additive.blend = BlendMode::Additive;
	ApplyRenderState(additive);
	bloomUp->UseShader();
	device.SetUniform(upLocs.texture, 0);
	for (int i = bloomLevelCount - 1; i > 0; i--)
	{
		const BloomLevel& src = bloomChain[i];
		const BloomLevel& dst = bloomChain[i - 1];
		device.BindFramebuffer(dst.fbo);
		device.SetViewport(0, 0, dst.width, dst.height);
		device.BindTexture(0, src.texture);
		device.SetUniform(upLocs.srcTexel, glm::vec2(1.0f / src.width, 1.0f / src.height));
		device.SetUniform(upLocs.dstSize, glm::vec2((float)dst.width, (float)dst.height));
		device.Draw(EmptyVao(), Primitive::Triangles, 0, 3);
	}

	device.SetViewport(0, 0, hdrWidth, hdrHeight);
	bloomReady = true;
}

// -------------------------------------------------------- shader adaptation

std::string AdaptFragmentShader(const std::string& source, bool& adapted)
{
	adapted = false;
	if (!linearOn || source.find("KINJO_LINEAR_OUTPUT") != std::string::npos)
		return source;

	const std::string code = StripComments(source);

	// The colour output: the one at location 0, else the first declared.
	static const std::regex outRe(
		R"((?:layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*)?\bout\s+(?:(?:lowp|mediump|highp)\s+)?(vec4|vec3)\s+(\w+)\s*;)");
	std::string outName, outType;
	for (auto it = std::sregex_iterator(code.begin(), code.end(), outRe); it != std::sregex_iterator(); ++it)
	{
		const std::smatch& m = *it;
		const bool explicitZero = m[1].matched && m[1].str() == "0";
		if (explicitZero || (outName.empty() && !m[1].matched))
		{
			outType = m[2].str();
			outName = m[3].str();
			if (explicitZero)
				break;
		}
	}
	static const std::regex mainRe(R"(\bvoid\s+main\s*\(\s*(?:void\s*)?\))");
	if (outName.empty() || !std::regex_search(code, mainRe))
		return source;   // no colour output (a depth-only pass): nothing to convert

	std::string out = std::regex_replace(source, mainRe, "void kinjo_user_main()");
	out +=
		"\n// --- added by the engine (render/ColorPipeline.cpp): this shader writes\n"
		"// gamma-space colour, so convert it when the target is linear ---\n"
		"layout(std140) uniform KinjoTarget { int kinjoTargetLinear; };\n"
		"vec3 kinjo_SrgbToLinear(vec3 c)\n"
		"{\n"
		"    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));\n"
		"}\n"
		"void main()\n"
		"{\n"
		"    kinjo_user_main();\n"
		"    if (kinjoTargetLinear != 0)\n";
	if (outType == "vec4")
		out += "        " + outName + " = vec4(kinjo_SrgbToLinear(clamp(" + outName + ".rgb, 0.0, 1.0)), clamp("
			+ outName + ".a, 0.0, 1.0));\n";
	else
		out += "        " + outName + " = kinjo_SrgbToLinear(clamp(" + outName + ", 0.0, 1.0));\n";
	out += "}\n";
	adapted = true;
	return out;
}

bool RequiresEngineShader(const std::string& path)
{
	if (!linearOn)
		return false;
	static const std::set<std::string> kLinearAware = {
		"scene3d.vert", "scene3d.frag", "scene3d_instanced.vert",
		"billboard3d.vert", "billboard3d.frag",
		"flash.vert", "flash.frag",
	};
	const size_t slash = path.find_last_of("/\\");
	const std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
	return kLinearAware.count(file) > 0;
}
