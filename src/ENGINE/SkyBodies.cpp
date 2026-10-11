#include "SkyBodies.h"
#include "Skybox.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "RenderState.h"
#include "UniformBlocks.h"
#include "UniformBufferCache.h"
#include "render/RenderDevice.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
	// File-local, so no exported class changes layout.
	bool sunOn = false;
	bool moonOn = false;
	glm::vec3 toSun = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 toMoon = glm::vec3(0.0f, 1.0f, 0.0f);
	SkySunLook sun;
	SkyMoonLook moon;

	// What image-based lighting was last told to capture.
	int captureVersion = 0;
	bool capturedOn = false;
	glm::vec3 capturedSun = glm::vec3(0.0f);
	SkySunLook capturedLook;

	ShaderProgram* shader = nullptr;
	bool shaderFailed = false;

	// The "Sky" block (shaders/sky_bodies.glsl): every member a vec4, so std140
	// lays it out as this does.
	struct SkyBlockData
	{
		glm::vec4 sun;          // toward the sun xyz; w: 1 when it is set
		glm::vec4 sunColour;    // rgb; w: the disc's brightness
		glm::vec4 sunShape;     // radius, glare, wide falloff, near falloff
		glm::vec4 glow;         // rgb; w: wide
		glm::vec4 glow2;        // near, horizon amount, dusk, -
		glm::vec4 horizon;      // rgb; w: -
		glm::vec4 moon;         // toward the moon xyz; w: brightness (0: none)
		glm::vec4 moonColour;   // rgb; w: radius
		glm::vec4 moonNorth;    // xyz; w: earthshine
		glm::vec4 moonGlow;     // rgb; w: glow
	};
	static_assert(sizeof(SkyBlockData) == 160, "Sky block must match shaders/sky_bodies.glsl");
	UniformBufferCache skyBlocks(UniformBlock::Sky, sizeof(SkyBlockData), 2);

	bool EnsureShader()
	{
		if (shaderFailed)
			return false;
		if (shader == nullptr)
		{
			shader = new ShaderProgram(-1, "data/shaders/sky.vert", "data/shaders/sky.frag");
			if (shader->GetID() == 0)
			{
				std::cout << "ERROR: the sky shaders failed to build; no sun or moon drawn" << std::endl;
				delete shader;
				shader = nullptr;
				shaderFailed = true;
				return false;
			}
		}
		return true;
	}

	// A step a cross-fade would show in the captured sky.
	bool LookDiffers(const SkySunLook& a, const SkySunLook& b)
	{
		auto far = [](float x, float y, float tolerance) { return std::fabs(x - y) > tolerance; };
		auto farColour = [&](const glm::vec3& x, const glm::vec3& y)
		{
			return far(x.r, y.r, 0.02f) || far(x.g, y.g, 0.02f) || far(x.b, y.b, 0.02f);
		};
		return far(a.glowWide, b.glowWide, 0.01f) || far(a.glowNear, b.glowNear, 0.01f)
			|| far(a.horizonAmount, b.horizonAmount, 0.02f) || far(a.dusk, b.dusk, 0.02f)
			|| far(a.wideFalloff, b.wideFalloff, 0.05f * b.wideFalloff)
			|| far(a.nearFalloff, b.nearFalloff, 0.05f * b.nearFalloff)
			|| farColour(a.glow, b.glow) || farColour(a.horizon, b.horizon);
	}
}

void SetSkySun(const glm::vec3& dir, const SkySunLook& look)
{
	if (glm::length(dir) < 1e-6f)
		return;
	toSun = glm::normalize(dir);
	sun = look;
	sunOn = true;
	// Image-based lighting holds the sun's glow: capture again once the sun has
	// moved half a degree, or its look has changed by a visible step.
	const float halfDegree = std::cos(0.5f * 3.14159265f / 180.0f);
	if (!capturedOn || glm::dot(toSun, capturedSun) < halfDegree || LookDiffers(sun, capturedLook))
	{
		captureVersion++;
		capturedOn = true;
		capturedSun = toSun;
		capturedLook = sun;
	}
}

void SetSkyMoon(const glm::vec3& dir, const SkyMoonLook& look)
{
	if (glm::length(dir) < 1e-6f)
		return;
	toMoon = glm::normalize(dir);
	moon = look;
	moonOn = true;
}

void ClearSkyBodies()
{
	sunOn = false;
	moonOn = false;
	if (capturedOn)
	{
		captureVersion++;
		capturedOn = false;
	}
}

bool SkySunInSky()
{
	return sunOn;
}

int SkyBodiesCaptureVersion()
{
	return captureVersion;
}

void BindSkyBlock()
{
	SkyBlockData d = {};
	d.sun = glm::vec4(toSun, sunOn ? 1.0f : 0.0f);
	d.sunColour = glm::vec4(sun.colour, std::max(0.0f, sun.disc));
	d.sunShape = glm::vec4(std::max(1e-5f, sun.radius), std::max(0.0f, sun.glare),
		std::max(0.0f, sun.wideFalloff), std::max(0.0f, sun.nearFalloff));
	d.glow = glm::vec4(sun.glow, std::max(0.0f, sun.glowWide));
	d.glow2 = glm::vec4(std::max(0.0f, sun.glowNear), glm::clamp(sun.horizonAmount, 0.0f, 1.0f),
		glm::clamp(sun.dusk, 0.0f, 1.0f), 0.0f);
	d.horizon = glm::vec4(sun.horizon, 0.0f);
	d.moon = glm::vec4(toMoon, moonOn ? std::max(0.0f, moon.brightness) : 0.0f);
	d.moonColour = glm::vec4(moon.colour, std::max(1e-5f, moon.radius));
	d.moonNorth = glm::vec4(moon.north, glm::clamp(moon.earthshine, 0.0f, 1.0f));
	d.moonGlow = glm::vec4(moon.glowColour, std::max(0.0f, moon.glow));
	skyBlocks.Bind(&d);
}

bool DrawSkyWithBodies(const Renderer& renderer, float radius, Texture* sky, Texture* next, float blend,
	const glm::vec3& tint)
{
	if ((!sunOn && !moonOn) || sky == nullptr || renderer.camera.useOrthoCamera
		|| Skybox::meshSkySphere == nullptr || !EnsureShader())
		return false;

	RenderDevice& device = Device();
	shader->UseShader();
	renderer.BindWorldCameraBlock();
	BindSkyBlock();
	const unsigned int id = shader->GetID();
	const ProgramHandle program(id);

	const bool fading = next != nullptr && blend > 0.0f;
	Texture* face = (moonOn && moon.face != nullptr) ? moon.face : nullptr;
	sky->UseTexture(0);
	(fading ? next : sky)->UseTexture(1);
	(face != nullptr ? face : sky)->UseTexture(2);
	device.SetUniform(device.UniformLocation(program, "skyTexture"), 0);
	device.SetUniform(device.UniformLocation(program, "nextTexture"), 1);
	device.SetUniform(device.UniformLocation(program, "moonFace"), 2);

	auto set = [&](const char* name, const glm::vec4& v)
	{
		device.SetUniform(ShaderProgram::DrawUniformLocation(id, name), v);
	};
	set("p0", glm::vec4(tint, fading ? std::min(blend, 1.0f) : 0.0f));
	set("p1", glm::vec4(radius, face != nullptr ? 1.0f : 0.0f,
		face != nullptr ? (float)std::max(1, face->GetWidth()) : 1.0f, 0.0f));

	// Where the panorama was: opaque, writing depth, so everything in the world
	// is drawn in front of it (Skybox::Render's own passes are skipped).
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Off;
	s.depthTest = true;
	s.depthCompare = CompareOp::LessEqual;
	s.depthWrite = true;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	Skybox::meshSkySphere->RenderMesh(0);
	renderer.drawCallsPerFrame++;
	return true;
}
