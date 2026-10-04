// Image-based lighting - see Environment.h. Backend-agnostic: GPU work goes
// through RenderDevice, shaders through ShaderProgram.

#include "Environment.h"
#include "RenderDevice.h"
#include "ColorPipeline.h"
#include "ProgramEvents.h"
#include "../Shader.h"
#include "../Texture.h"
#include "../RenderState.h"
#include "../UniformBlocks.h"
#include "../UniformBufferCache.h"
#include "../globals.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace
{
	// --- settings -----------------------------------------------------------
	float projectDiffuse = 1.0f;     // renderer.dat `ibl`
	float projectSpecular = 1.0f;    // renderer.dat `iblSpecular` (defaults to `ibl`)
	int forced = -1;                 // KINJO_IBL: 0 = off, 1 = on, -1 = not set
	float sceneDiffuse = -1.0f;      // < 0 = project default
	float sceneSpecular = -1.0f;

	float EffectiveDiffuse()
	{
		if (forced == 0)
			return 0.0f;
		float v = (sceneDiffuse >= 0.0f) ? sceneDiffuse : projectDiffuse;
		return (forced == 1 && v <= 0.0f) ? 1.0f : v;
	}

	float EffectiveSpecular()
	{
		if (forced == 0)
			return 0.0f;
		float v = (sceneSpecular >= 0.0f) ? sceneSpecular : projectSpecular;
		return (forced == 1 && v <= 0.0f) ? 1.0f : v;
	}

	// --- state --------------------------------------------------------------
	bool active = false;   // IBL lights this frame
	bool dirty = false;    // the maps no longer match the sky
	bool ready = false;    // the maps hold a captured sky
	SkySource current;

	// What the maps were built from. Blend and tint are quantised so a slow
	// time-of-day cross-fade rebuilds in small steps, not every frame.
	struct Signature
	{
		Texture* sky = nullptr;
		Texture* next = nullptr;
		int blend = 0;
		int r = 0, g = 0, b = 0;
		bool operator==(const Signature& o) const
		{
			return sky == o.sky && next == o.next && blend == o.blend && r == o.r && g == o.g && b == o.b;
		}
	};
	Signature lastSignature;
	bool haveSignature = false;

	Signature SignatureOf(const SkySource& s)
	{
		Signature sig;
		sig.sky = s.sky;
		sig.next = (s.blend > 0.0f) ? s.next : nullptr;
		sig.blend = (sig.next != nullptr) ? (int)std::lround(std::min(std::max(s.blend, 0.0f), 1.0f) * 64.0f) : 0;
		sig.r = (int)std::lround(s.tint.r * 255.0f);
		sig.g = (int)std::lround(s.tint.g * 255.0f);
		sig.b = (int)std::lround(s.tint.b * 255.0f);
		return sig;
	}

	// --- resources ----------------------------------------------------------
	const int kBaseWidth = 512, kBaseHeight = 256, kBaseLevels = 10;     // down to 1x1... 2x1
	const int kIrradianceWidth = 64, kIrradianceHeight = 32;
	const int kSpecularWidth = 256, kSpecularHeight = 128, kSpecularLevels = 6;
	const int kLutSize = 128;
	const float kIrradianceSourceLod = 4.0f;   // 32x16 of the base: smooth enough for 256 samples

	TextureHandle envBase;
	TextureHandle envIrradiance;
	TextureHandle envSpecular;
	TextureHandle brdfLut;
	FramebufferHandle fbo;
	VertexArrayHandle emptyVao;
	bool lutReady = false;

	struct Pass
	{
		ShaderProgram* shader = nullptr;
		int dstSize = -1, a = -1, b = -1, c = -1;   // pass-specific uniform locations
		int tex0 = -1, tex1 = -1;
	};
	Pass capture, irradiance, specular, lut;
	bool shadersFailed = false;

	struct EnvironmentBlockData
	{
		int on;
		float diffuse;
		float specular;
		float maxLod;
	};
	UniformBufferCache environmentBlocks(UniformBlock::Environment, sizeof(EnvironmentBlockData), 2);

	struct ProgramLocs { int irradiance = -1, specular = -1, lut = -1; };
	std::unordered_map<unsigned int, ProgramLocs> locsByProgram;

	bool BuildPass(Pass& p, const char* frag, const char* name)
	{
		p.shader = new ShaderProgram(-1, "data/shaders/resolve.vert", frag);
		p.shader->SetNameString(name);
		return p.shader->GetID() != 0;
	}

	bool EnsureShaders()
	{
		if (shadersFailed)
			return false;
		if (capture.shader != nullptr)
			return true;
		const bool ok = BuildPass(capture, "data/shaders/env_capture.frag", "env_capture")
			& BuildPass(irradiance, "data/shaders/env_irradiance.frag", "env_irradiance")
			& BuildPass(specular, "data/shaders/env_specular.frag", "env_specular")
			& BuildPass(lut, "data/shaders/brdf_lut.frag", "brdf_lut");
		if (!ok)
		{
			std::cout << "ERROR: image-based lighting shaders failed to build; IBL disabled" << std::endl;
			for (Pass* p : { &capture, &irradiance, &specular, &lut })
			{
				delete p->shader;
				p->shader = nullptr;
			}
			shadersFailed = true;
			return false;
		}

		RenderDevice& device = Device();
		auto loc = [&](const Pass& p, const char* n) { return ShaderProgram::DrawUniformLocation(p.shader->GetID(), n); };
		auto sampler = [&](const Pass& p, const char* n) { return device.UniformLocation(ProgramHandle(p.shader->GetID()), n); };
		capture.dstSize = loc(capture, "dstSize");
		capture.a = loc(capture, "tint");
		capture.b = loc(capture, "blend");
		capture.tex0 = sampler(capture, "skyTexture");
		capture.tex1 = sampler(capture, "nextTexture");
		irradiance.dstSize = loc(irradiance, "dstSize");
		irradiance.a = loc(irradiance, "srcLod");
		irradiance.tex0 = sampler(irradiance, "envBase");
		specular.dstSize = loc(specular, "dstSize");
		specular.a = loc(specular, "roughness");
		specular.b = loc(specular, "srcWidth");
		specular.tex0 = sampler(specular, "envBase");
		lut.dstSize = loc(lut, "dstSize");
		return true;
	}

	TextureHandle MakeTarget(int w, int h, int levels, TextureFilter filter, TextureWrap wrap)
	{
		TextureDesc desc;
		desc.format = TextureFormat::RGBA16F;
		desc.width = w;
		desc.height = h;
		desc.mipLevels = levels;
		desc.filter = filter;
		desc.wrap = wrap;
		return Device().CreateTexture(desc);
	}

	void EnsureTargets()
	{
		if (envBase)
			return;
		RenderDevice& device = Device();
		// Panoramas wrap around horizontally (Repeat); the poles are averaged anyway.
		envBase = MakeTarget(kBaseWidth, kBaseHeight, kBaseLevels, TextureFilter::Trilinear, TextureWrap::Repeat);
		envIrradiance = MakeTarget(kIrradianceWidth, kIrradianceHeight, 1, TextureFilter::Linear, TextureWrap::Repeat);
		envSpecular = MakeTarget(kSpecularWidth, kSpecularHeight, kSpecularLevels, TextureFilter::Trilinear, TextureWrap::Repeat);
		brdfLut = MakeTarget(kLutSize, kLutSize, 1, TextureFilter::Linear, TextureWrap::ClampToEdge);
		fbo = device.CreateFramebuffer();
		emptyVao = device.CreateVertexArray();
	}

	// Point the work framebuffer at one level of a target and fill it.
	void DrawInto(TextureHandle target, int level, int w, int h)
	{
		RenderDevice& device = Device();
		device.AttachTexture(fbo, Attachment::Color0, target, TextureType::Tex2D, 0, level);
		device.BindFramebuffer(fbo);
		device.SetViewport(0, 0, w, h);
		device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	}
}

void LoadEnvironmentSettings()
{
	auto config = GetMapStringsFromFile("data/config/renderer.dat");
	auto parse = [](const std::string& s, float fallback) -> float
	{
		try { return std::max(std::stof(s), 0.0f); }
		catch (...) { return fallback; }
	};
	projectDiffuse = (config.count("ibl") > 0) ? parse(config["ibl"], 1.0f) : 1.0f;
	projectSpecular = (config.count("iblSpecular") > 0) ? parse(config["iblSpecular"], projectDiffuse) : projectDiffuse;
	if (const char* e = std::getenv("KINJO_IBL"))
		forced = (e[0] == '0') ? 0 : (e[0] == '1') ? 1 : -1;
	if (LinearWorkflow())
		std::cout << "Image-based lighting: diffuse " << projectDiffuse << ", specular " << projectSpecular
			<< (forced == 0 ? " (KINJO_IBL=0: off)" : forced == 1 ? " (KINJO_IBL=1: forced on)" : "") << std::endl;
}

void SetEnvironmentSource(bool hasSky, const SkySource& source)
{
	const bool want = LinearWorkflow() && hasSky && source.sky != nullptr && !shadersFailed
		&& (EffectiveDiffuse() > 0.0f || EffectiveSpecular() > 0.0f);
	if (!want)
	{
		active = false;
		dirty = false;
		return;
	}
	active = true;
	const Signature sig = SignatureOf(source);
	if (!ready || !haveSignature || !(sig == lastSignature))
	{
		dirty = true;
		current = source;
		lastSignature = sig;
		haveSignature = true;
	}
}

bool EnvironmentDirty()
{
	return active && dirty;
}

bool EnvironmentActive()
{
	return active && ready;
}

void UpdateEnvironment(int screenWidth, int screenHeight)
{
	dirty = false;
	if (!active || current.sky == nullptr || !EnsureShaders())
		return;
	EnsureTargets();

	RenderDevice& device = Device();
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Off;
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	// The BRDF table depends on nothing in the scene: once.
	if (!lutReady)
	{
		lut.shader->UseShader();
		device.SetUniform(lut.dstSize, glm::vec2((float)kLutSize, (float)kLutSize));
		DrawInto(brdfLut, 0, kLutSize, kLutSize);
		lutReady = true;
	}

	// 1. The sky (cross-fade and tint included), in linear light.
	capture.shader->UseShader();
	current.sky->UseTexture(0);
	Texture* next = (current.next != nullptr && current.blend > 0.0f) ? current.next : current.sky;
	next->UseTexture(1);
	device.SetUniform(capture.tex0, 0);
	device.SetUniform(capture.tex1, 1);
	device.SetUniform(capture.a, glm::vec4(SceneColor(current.tint), 1.0f));
	device.SetUniform(capture.b, (next != current.sky) ? std::min(std::max(current.blend, 0.0f), 1.0f) : 0.0f);
	device.SetUniform(capture.dstSize, glm::vec2((float)kBaseWidth, (float)kBaseHeight));
	DrawInto(envBase, 0, kBaseWidth, kBaseHeight);
	device.GenerateMipmaps(envBase);

	// 2. Diffuse irradiance.
	irradiance.shader->UseShader();
	device.BindTexture(0, envBase);
	device.SetUniform(irradiance.tex0, 0);
	device.SetUniform(irradiance.a, kIrradianceSourceLod);
	device.SetUniform(irradiance.dstSize, glm::vec2((float)kIrradianceWidth, (float)kIrradianceHeight));
	DrawInto(envIrradiance, 0, kIrradianceWidth, kIrradianceHeight);

	// 3. Specular, one roughness per level.
	specular.shader->UseShader();
	device.BindTexture(0, envBase);
	device.SetUniform(specular.tex0, 0);
	device.SetUniform(specular.b, (float)kBaseWidth);
	for (int level = 0; level < kSpecularLevels; level++)
	{
		const int w = std::max(1, kSpecularWidth >> level);
		const int h = std::max(1, kSpecularHeight >> level);
		device.SetUniform(specular.a, (float)level / (float)(kSpecularLevels - 1));
		device.SetUniform(specular.dstSize, glm::vec2((float)w, (float)h));
		DrawInto(envSpecular, level, w, h);
	}

	// Debugging: KINJO_DUMP_ENV=<folder> writes the maps of the first capture as
	// PNGs (linear values clamped to 1 and sRGB-encoded for viewing).
	static bool dumped = false;
	if (!dumped)
	{
		if (const char* dir = std::getenv("KINJO_DUMP_ENV"))
		{
			dumped = true;
			auto dump = [&](TextureHandle tex, int level, int w, int h, const std::string& name)
			{
				device.AttachTexture(fbo, Attachment::Color0, tex, TextureType::Tex2D, 0, level);
				device.BindFramebuffer(fbo);
				std::vector<unsigned char> px((size_t)w * h * 4);
				device.ReadPixels(0, 0, w, h, ReadbackFormat::RGBA8, px.data());
				SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
				for (int y = 0; y < h; y++)
				{
					unsigned char* row = (unsigned char*)surface->pixels + (size_t)y * surface->pitch;
					const unsigned char* src = px.data() + (size_t)y * w * 4;   // same row order as the texture (v = 0 first)
					for (int x = 0; x < w * 4; x++)
					{
						const float lin = src[x] / 255.0f;
						const float enc = (x % 4 == 3) ? lin
							: (lin <= 0.0031308f ? lin * 12.92f : 1.055f * std::pow(lin, 1.0f / 2.4f) - 0.055f);
						row[x] = (unsigned char)std::lround(std::min(std::max(enc, 0.0f), 1.0f) * 255.0f);
					}
				}
				const std::string path = std::string(dir) + "/" + name + ".png";
				IMG_SavePNG(surface, path.c_str());
				SDL_FreeSurface(surface);
				std::cout << "Image-based lighting: wrote " << path << std::endl;
			};
			dump(envBase, 0, kBaseWidth, kBaseHeight, "env_base");
			dump(envIrradiance, 0, kIrradianceWidth, kIrradianceHeight, "env_irradiance");
			for (int level = 0; level < kSpecularLevels; level++)
				dump(envSpecular, level, std::max(1, kSpecularWidth >> level), std::max(1, kSpecularHeight >> level),
					"env_specular_" + std::to_string(level));
			dump(brdfLut, 0, kLutSize, kLutSize, "brdf_lut");
		}
	}

	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, screenWidth, screenHeight);
	ready = true;
}

void BindEnvironment(unsigned int program)
{
	EnvironmentBlockData data = {};
	data.on = EnvironmentActive() ? 1 : 0;
	data.diffuse = EffectiveDiffuse();
	data.specular = EffectiveSpecular();
	data.maxLod = (float)(kSpecularLevels - 1);
	environmentBlocks.Bind(&data);
	if (data.on == 0 || program == 0)
		return;

	RenderDevice& device = Device();
	auto it = locsByProgram.find(program);
	if (it == locsByProgram.end())
	{
		// Forget a program's entry when it is deleted (GL reuses ids).
		AddProgramDeletedListener([](unsigned int p) { locsByProgram.erase(p); });
		ProgramLocs l;
		l.irradiance = device.UniformLocation(ProgramHandle(program), "envIrradiance");
		l.specular = device.UniformLocation(ProgramHandle(program), "envSpecular");
		l.lut = device.UniformLocation(ProgramHandle(program), "brdfLUT");
		it = locsByProgram.emplace(program, l).first;
	}
	device.BindTexture(8, envIrradiance);
	device.BindTexture(9, envSpecular);
	device.BindTexture(10, brdfLut);
	device.SetUniform(it->second.irradiance, 8);
	device.SetUniform(it->second.specular, 9);
	device.SetUniform(it->second.lut, 10);
}

void SetSceneIBL(float diffuse, float specular)
{
	sceneDiffuse = (diffuse < 0.0f) ? -1.0f : diffuse;
	sceneSpecular = (specular < 0.0f) ? -1.0f : specular;
}

void GetProjectIBL(float& diffuse, float& specular)
{
	diffuse = projectDiffuse;
	specular = projectSpecular;
}

void GetSceneIBL(float& diffuse, float& specular)
{
	diffuse = sceneDiffuse;
	specular = sceneSpecular;
}

void ReleaseEnvironment()
{
	RenderDevice& device = Device();
	for (TextureHandle* t : { &envBase, &envIrradiance, &envSpecular, &brdfLut })
		if (*t)
			device.DestroyTexture(*t);
	if (fbo)
		device.DestroyFramebuffer(fbo);
	if (emptyVao)
		device.DestroyVertexArray(emptyVao);
	for (Pass* p : { &capture, &irradiance, &specular, &lut })
	{
		delete p->shader;
		p->shader = nullptr;
	}
	locsByProgram.clear();
	active = dirty = ready = lutReady = haveSignature = false;
	shadersFailed = false;
}
