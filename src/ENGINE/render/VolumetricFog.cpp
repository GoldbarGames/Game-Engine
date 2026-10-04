// Volumetric fog - see VolumetricFog.h. Backend-agnostic: GPU work goes
// through RenderDevice, shaders through ShaderProgram.

#include "VolumetricFog.h"
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
#include <sstream>

namespace
{
	const int kSteps = 24;

	// --- settings -----------------------------------------------------------
	bool allowed = true;            // renderer.dat `volumetricFog`
	FogSettings project;            // renderer.dat fog* (density 0 = none)
	float reach = 6000.0f;          // renderer.dat `fogDistance`: how far rays march
	bool sceneSet = false;          // the scene's own fog (.scene `fog`)
	FogSettings scene;
	float weatherDensity = 0.0f;    // rain/snow/storm, when the scene sets none
	bool forced = false;            // KINJO_FOG
	FogSettings forcedFog;

	// A density fade (scene3d fog <density> <seconds>).
	float fadeFrom = 0.0f;
	float fadeSeconds = 0.0f;
	std::chrono::steady_clock::time_point fadeStart;
	unsigned int frameIndex = 0;

	// --- resources ----------------------------------------------------------
	int width = 0, height = 0;
	TextureHandle fogTexture;
	FramebufferHandle fogFbo;
	VertexArrayHandle emptyVao;
	ShaderProgram* march = nullptr;
	ShaderProgram* composite = nullptr;
	bool failed = false;
	struct Locs { int invViewProj = -1, medium = -1, tint = -1, params = -1, halfSize = -1, fullSize = -1,
		planes = -1, depth = -1, fog = -1; };
	Locs marchLocs, compositeLocs;

	// The settings in force (before any fade).
	FogSettings Target()
	{
		if (forced)
			return forcedFog;
		if (sceneSet)
			return scene;
		FogSettings f = project;
		if (f.density <= 0.0f && weatherDensity > 0.0f)
		{
			f.density = weatherDensity;
			f.noise = std::max(f.noise, 0.35f);   // weather fog drifts
		}
		return f;
	}

	FogSettings Current()
	{
		FogSettings f = Target();
		if (fadeSeconds > 0.0f)
		{
			const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - fadeStart).count() / fadeSeconds;
			if (t < 1.0f)
				f.density = fadeFrom + (f.density - fadeFrom) * t;
			else
				fadeSeconds = 0.0f;
		}
		return f;
	}

	void StartFade(float seconds)
	{
		fadeFrom = Current().density;
		fadeSeconds = std::max(seconds, 0.0f);
		fadeStart = std::chrono::steady_clock::now();
	}

	bool EnsureShaders()
	{
		if (failed)
			return false;
		if (march != nullptr)
			return true;
		march = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/fog_march.frag");
		march->SetNameString("fog_march");
		composite = new ShaderProgram(-1, "data/shaders/resolve.vert", "data/shaders/fog_composite.frag");
		composite->SetNameString("fog_composite");
		if (march->GetID() == 0 || composite->GetID() == 0)
		{
			std::cout << "ERROR: volumetric fog shaders failed to build; fog disabled" << std::endl;
			delete march;
			delete composite;
			march = composite = nullptr;
			failed = true;
			return false;
		}
		RenderDevice& device = Device();
		auto loc = [](ShaderProgram* s, const char* n) { return ShaderProgram::DrawUniformLocation(s->GetID(), n); };
		marchLocs.invViewProj = loc(march, "invViewProj");
		marchLocs.medium = loc(march, "medium");
		marchLocs.tint = loc(march, "tint");
		marchLocs.params = loc(march, "params");
		marchLocs.halfSize = loc(march, "halfSize");
		marchLocs.fullSize = loc(march, "fullSize");
		marchLocs.depth = device.UniformLocation(ProgramHandle(march->GetID()), "sceneDepth");
		compositeLocs.planes = loc(composite, "planes");
		compositeLocs.fullSize = loc(composite, "fullSize");
		compositeLocs.halfSize = loc(composite, "halfSize");
		compositeLocs.depth = device.UniformLocation(ProgramHandle(composite->GetID()), "sceneDepth");
		compositeLocs.fog = device.UniformLocation(ProgramHandle(composite->GetID()), "fogTexture");
		return true;
	}

	void EnsureTargets(int w, int h)
	{
		if (fogTexture && w == width && h == height)
			return;
		RenderDevice& device = Device();
		if (fogTexture)
			device.DestroyTexture(fogTexture);
		if (fogFbo)
			device.DestroyFramebuffer(fogFbo);
		TextureDesc desc;
		desc.format = TextureFormat::RGBA16F;
		desc.width = std::max(w / 2, 1);
		desc.height = std::max(h / 2, 1);
		desc.filter = TextureFilter::Nearest;   // the composite does its own depth-aware filtering
		desc.wrap = TextureWrap::ClampToEdge;
		fogTexture = device.CreateTexture(desc);
		fogFbo = device.CreateFramebuffer();
		device.AttachTexture(fogFbo, Attachment::Color0, fogTexture);
		if (!emptyVao)
			emptyVao = device.CreateVertexArray();
		device.BindFramebuffer(FramebufferHandle());
		width = w;
		height = h;
	}

	bool ParseFloat(const std::string& s, float& out)
	{
		try { out = std::stof(s); return true; }
		catch (...) { return false; }
	}
}

void LoadFogSettings()
{
	auto config = GetMapStringsFromFile("data/config/renderer.dat");
	allowed = !(config.count("volumetricFog") > 0 && config["volumetricFog"] == "0");
	float v = 0.0f;
	if (config.count("fog") > 0 && ParseFloat(config["fog"], v)) project.density = std::max(v, 0.0f);
	if (config.count("fogHeightFalloff") > 0 && ParseFloat(config["fogHeightFalloff"], v)) project.heightFalloff = std::max(v, 0.0f);
	if (config.count("fogAnisotropy") > 0 && ParseFloat(config["fogAnisotropy"], v)) project.anisotropy = std::min(std::max(v, -0.95f), 0.95f);
	if (config.count("fogNoise") > 0 && ParseFloat(config["fogNoise"], v)) project.noise = std::min(std::max(v, 0.0f), 1.0f);
	if (config.count("fogDistance") > 0 && ParseFloat(config["fogDistance"], v)) reach = std::max(v, 100.0f);
	if (config.count("fogColor") > 0)
	{
		std::istringstream ss(config["fogColor"]);
		glm::vec3 c;
		if (ss >> c.r >> c.g >> c.b)
			project.color = c;
	}
	if (const char* e = std::getenv("KINJO_FOG"))
	{
		forcedFog = project;
		float density = 0.0f, falloff = -1.0f;
		const int n = std::sscanf(e, "%f %f", &density, &falloff);
		if (n >= 1)
		{
			forced = true;
			forcedFog.density = std::max(density, 0.0f);
			if (n >= 2 && falloff >= 0.0f)
				forcedFog.heightFalloff = falloff;
			if (LinearWorkflow())
				std::cout << "Volumetric fog: KINJO_FOG density " << forcedFog.density << ", falloff "
					<< forcedFog.heightFalloff << std::endl;
		}
	}
}

void SetSceneFog(bool set, const FogSettings& fog, float seconds)
{
	StartFade(seconds);
	sceneSet = set;
	scene = fog;
}

FogSettings FogInForce()
{
	return Target();
}

bool GetSceneFog(FogSettings& fog)
{
	fog = scene;
	return sceneSet;
}

void SetSceneFogDensity(float density, float seconds)
{
	StartFade(seconds);
	if (!sceneSet)
	{
		scene = Target();
		sceneSet = true;
	}
	scene.density = std::max(density, 0.0f);
}

void SetWeatherFogDensity(float density)
{
	weatherDensity = std::max(density, 0.0f);
}

bool FogActive()
{
	return LinearWorkflow() && allowed && !failed && Current().density > 0.0f;
}

unsigned int BeginFogMarch(int w, int h)
{
	if (!FogActive() || w <= 0 || h <= 0 || !EnsureShaders())
		return 0;
	EnsureTargets(w, h);
	return march->GetID();
}

void DrawFogMarch(const glm::mat4& invViewProj, TextureHandle depth, int w, int h, float timeSeconds)
{
	if (march == nullptr || !fogTexture)
		return;
	const FogSettings f = Current();
	const int hw = std::max(w / 2, 1), hh = std::max(h / 2, 1);
	RenderDevice& device = Device();
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Off;
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	device.BindFramebuffer(fogFbo);
	device.SetViewport(0, 0, hw, hh);
	device.BindTexture(15, depth);
	device.SetUniform(marchLocs.depth, 15);
	device.SetUniform(marchLocs.invViewProj, invViewProj);
	device.SetUniform(marchLocs.medium, glm::vec4(f.density, f.heightFalloff, f.baseHeight, f.anisotropy));
	device.SetUniform(marchLocs.tint, glm::vec4(SceneColor(f.color), f.noise));
	device.SetUniform(marchLocs.params, glm::vec4(reach, (float)kSteps, (float)(frameIndex++ % 64), timeSeconds));
	device.SetUniform(marchLocs.halfSize, glm::vec2((float)hw, (float)hh));
	device.SetUniform(marchLocs.fullSize, glm::vec2((float)w, (float)h));
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	device.BindFramebuffer(FramebufferHandle());
}

void CompositeFog(TextureHandle depth, float nearPlane, float farPlane, int w, int h)
{
	if (composite == nullptr || !fogTexture || !BindWorldColorOnly())
		return;
	RenderDevice& device = Device();
	RenderState s = CurrentRenderState();
	// A premultiplied layer: world * transmittance + scattered light, and the
	// fog's opacity added to the world's alpha (fog fills cracks and empty
	// background too).
	s.blend = BlendMode::Premultiplied;
	s.depthTest = false;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);
	composite->UseShader();
	device.BindTexture(0, fogTexture);
	device.BindTexture(1, depth);
	device.SetUniform(compositeLocs.fog, 0);
	device.SetUniform(compositeLocs.depth, 1);
	device.SetUniform(compositeLocs.planes, glm::vec4(nearPlane, farPlane, 0.0f, 0.0f));
	device.SetUniform(compositeLocs.fullSize, glm::vec2((float)w, (float)h));
	device.SetUniform(compositeLocs.halfSize, glm::vec2((float)std::max(w / 2, 1), (float)std::max(h / 2, 1)));
	device.Draw(emptyVao, Primitive::Triangles, 0, 3);
	device.BindFramebuffer(FramebufferHandle());
	SetTargetLinear(false);
}

void ReleaseVolumetricFog()
{
	RenderDevice& device = Device();
	if (fogTexture)
		device.DestroyTexture(fogTexture);
	if (fogFbo)
		device.DestroyFramebuffer(fogFbo);
	if (emptyVao)
		device.DestroyVertexArray(emptyVao);
	delete march;
	delete composite;
	march = composite = nullptr;
	failed = false;
	width = height = 0;
}
