#include "SkyClouds.h"
#include "Wind.h"
#include "Skybox.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "RenderState.h"
#include "render/RenderDevice.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
	// File-local, so no exported class changes layout.
	const int MAX_LAYERS = 2;
	SkyCloudLayer layers[MAX_LAYERS];
	int layerCount = 0;
	glm::vec3 toSun = glm::normalize(glm::vec3(0.47f, -0.56f, 0.68f));
	glm::vec3 toRim = toSun;
	float glare = 0.6f;
	glm::vec2 windAloft = glm::vec2(12.0f, 4.0f);
	glm::dvec2 drift[MAX_LAYERS];     // how far each layer has blown, world units
	unsigned int frame = 0;           // turns the march's stagger each frame (TAA averages it)

	ShaderProgram* shader = nullptr;
	bool shaderFailed = false;
	TextureHandle noise;
	const int NOISE = 256;            // the table: shaders/sky_clouds.frag reads it repeating

	// The noise table: a value at each lattice point, read smoothly between
	// them by the shader (as tools/make_sky.py's Noise2 does from its own).
	bool EnsureResources()
	{
		if (shaderFailed)
			return false;
		if (shader == nullptr)
		{
			shader = new ShaderProgram(-1, "data/shaders/sky_clouds.vert", "data/shaders/sky_clouds.frag");
			if (shader->GetID() == 0)
			{
				std::cout << "ERROR: the sky's cloud shaders failed to build; no moving clouds" << std::endl;
				delete shader;
				shader = nullptr;
				shaderFailed = true;
				return false;
			}
		}
		if (!noise)
		{
			std::vector<uint8_t> values((size_t)NOISE * NOISE);
			uint32_t s = 0x9E3779B9u;
			for (uint8_t& v : values)
			{
				s ^= s << 13;
				s ^= s >> 17;
				s ^= s << 5;
				v = (uint8_t)(s >> 24);
			}
			TextureDesc desc;
			desc.format = TextureFormat::R8;
			desc.width = NOISE;
			desc.height = NOISE;
			desc.filter = TextureFilter::Linear;   // the shader reads between lattice points smoothly
			desc.wrap = TextureWrap::Repeat;
			noise = Device().CreateTexture(desc, values.data());
		}
		return true;
	}
}

void SetSkyClouds(const SkyCloudLayer* in, int count)
{
	layerCount = std::max(0, std::min(MAX_LAYERS, count));
	for (int i = 0; i < layerCount; i++)
		layers[i] = in[i];
}

void SetSkyCloudLight(const glm::vec3& sun, const glm::vec3& rim, float g)
{
	if (glm::length(sun) > 1e-6f)
		toSun = glm::normalize(sun);
	toRim = (glm::length(rim) > 1e-6f) ? glm::normalize(rim) : toSun;
	glare = std::max(0.0f, g);
}

void SetSkyCloudWind(const glm::vec2& velocityXZ)
{
	windAloft = velocityXZ;
}

int SkyCloudLayers()
{
	return layerCount;
}

void UpdateSkyClouds(float realSeconds)
{
	const double dt = (double)std::max(0.0f, realSeconds) * WindTimeScale();
	frame++;
	for (int i = 0; i < MAX_LAYERS; i++)
	{
		const float speed = (i < layerCount) ? layers[i].speed : 1.0f;
		drift[i] += glm::dvec2(windAloft) * (double)speed * dt;
	}
}

void DrawSkyClouds(const Renderer& renderer, float radius)
{
	if (layerCount == 0 || renderer.camera.useOrthoCamera || Skybox::meshSkySphere == nullptr || !EnsureResources())
		return;
	RenderDevice& device = Device();
	shader->UseShader();
	renderer.BindWorldCameraBlock();
	const unsigned int id = shader->GetID();
	device.SetUniform(device.UniformLocation(ProgramHandle(id), "cloudNoise"), 0);
	device.BindTexture(0, noise);

	// Over the panorama on the same sphere, blended (premultiplied), depth-
	// tested against it but not written.
	RenderState s = CurrentRenderState();
	s.blend = BlendMode::Premultiplied;
	s.depthTest = true;
	s.depthCompare = CompareOp::LessEqual;
	s.depthWrite = false;
	s.cull = CullMode::None;
	ScopedRenderState scope(s);

	auto set = [&](const char* name, const glm::vec4& v)
	{
		device.SetUniform(ShaderProgram::DrawUniformLocation(id, name), v);
	};
	for (int i = 0; i < layerCount; i++)
	{
		const SkyCloudLayer& L = layers[i];
		set("p0", glm::vec4(L.heaped ? 1.0f : 0.0f, std::max(1.0f, L.height), std::max(1.0f, L.size), L.cover));
		set("p1", glm::vec4(std::max(0.001f, L.soft), L.warp, L.opacity, L.depth));
		set("p2", glm::vec4(L.stretch.x, L.stretch.y, L.angle, std::max(1.0f, L.hazeDistance)));
		set("p3", glm::vec4(L.lit, L.silver));
		set("p4", glm::vec4(L.shade, L.thickDark));
		// The noise seed in the whole part, this frame's turn of the stagger in
		// the fraction (golden-ratio steps: evenly spread over a few frames).
		const float turn = (float)std::fmod((double)frame * 0.6180339887, 1.0);
		set("p5", glm::vec4((float)drift[i].x, (float)drift[i].y, L.baseDark, (float)(i * 11) + 0.999f * turn));
		// A little inside the panorama's sphere: drawn mirrored (Skybox::Render),
		// its triangles are not this mesh's, and on the same radius the two
		// z-fought - the clouds came out in bands and checks where the sky
		// won. At 0.995 every one of these triangles is in front of every one
		// of the sky's, and still behind everything in the world.
		set("p6", glm::vec4(toSun, radius * 0.995f));
		set("p7", glm::vec4(toRim, glare));
		Skybox::meshSkySphere->RenderMesh(0);
		renderer.drawCallsPerFrame++;
	}
}
