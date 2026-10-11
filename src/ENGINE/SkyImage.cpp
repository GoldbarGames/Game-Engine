#include "SkyImage.h"
#include "Scene3D.h"
#include "Skybox.h"
#include "Game.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "RenderState.h"
#include "render/RenderDevice.h"

#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

// A sky the game painted itself (Scene3D::SetSkyImage), kept here so no
// exported class changes layout.
namespace
{
	TextureHandle texture;
	int width = 0, height = 0;
	int version = 0;
	ShaderProgram* shader = nullptr;
	bool shaderFailed = false;

	// The sun's disc over it (Scene3D::SetSkyImageSun)
	struct Sun
	{
		glm::vec3 toSun = glm::vec3(0.0f, -1.0f, 0.0f);
		float radius = 0.0047f;
		glm::vec3 radiance = glm::vec3(0.0f);
		float flatten = 1.0f;
		glm::vec3 lowerTint = glm::vec3(1.0f), upperTint = glm::vec3(1.0f);
	} sun;

	bool EnsureShader()
	{
		if (shader != nullptr)
			return true;
		if (shaderFailed)
			return false;
		shader = new ShaderProgram(-1, "data/shaders/sky_hdr.vert", "data/shaders/sky_hdr.frag");
		shader->SetNameString("sky_hdr");
		if (shader->GetID() == 0)
		{
			std::cout << "ERROR: the HDR sky shader failed to build; the painted sky won't show" << std::endl;
			delete shader;
			shader = nullptr;
			shaderFailed = true;
			return false;
		}
		return true;
	}
}

void Scene3D::SetSkyImage(Game& game, const float* rgb, int w, int h)
{
	RenderDevice& device = Device();
	if (rgb == nullptr || w < 2 || h < 1)
	{
		// Back to the scene's own sky
		if (texture)
			device.DestroyTexture(texture);
		width = height = 0;
		version++;
		return;
	}

	// RGBA16F: the game's floats as halves, alpha 1
	std::vector<uint16_t> halves((size_t)w * h * 4);
	const uint16_t one = glm::packHalf1x16(1.0f);
	for (size_t i = 0, n = (size_t)w * h; i < n; i++)
	{
		halves[i * 4 + 0] = glm::packHalf1x16(std::max(rgb[i * 3 + 0], 0.0f));
		halves[i * 4 + 1] = glm::packHalf1x16(std::max(rgb[i * 3 + 1], 0.0f));
		halves[i * 4 + 2] = glm::packHalf1x16(std::max(rgb[i * 3 + 2], 0.0f));
		halves[i * 4 + 3] = one;
	}
	if (texture && w == width && h == height)
	{
		device.UpdateTexture(texture, TextureFormat::RGBA16F, 0, 0, w, h, halves.data());
#ifndef __EMSCRIPTEN__
		device.GenerateMipmaps(texture);
#endif
	}
	else
	{
		if (texture)
			device.DestroyTexture(texture);
		TextureDesc desc;
		desc.format = TextureFormat::RGBA16F;
		desc.width = w;
		desc.height = h;
		desc.wrap = TextureWrap::Repeat;   // around the horizon (the poles are drawn by direction)
#ifdef __EMSCRIPTEN__
		desc.filter = TextureFilter::Linear;   // WebGL2 builds float mips only where it can render to them
#else
		desc.filter = TextureFilter::Trilinear;
		desc.generateMipmaps = true;
#endif
		texture = device.CreateTexture(desc, halves.data());
		width = w;
		height = h;
	}
	version++;

	// A scene without a sky gets the sphere to draw it on
	EnsureSkyForImage(game);
}

bool Scene3D::HasSkyImage() const
{
	return (bool)texture;
}

void Scene3D::SetSkyImageSun(const glm::vec3& toSun, float angularRadius, const glm::vec3& radiance, float flatten,
	const glm::vec3& lowerTint, const glm::vec3& upperTint)
{
	const float len = glm::length(toSun);
	sun.toSun = (len > 1e-6f) ? toSun / len : glm::vec3(0.0f, -1.0f, 0.0f);
	sun.radius = std::max(angularRadius, 1e-4f);
	sun.radiance = glm::max(radiance, glm::vec3(0.0f));
	sun.flatten = std::max(flatten, 0.05f);
	sun.lowerTint = lowerTint;
	sun.upperTint = upperTint;
}

void Scene3D::EnsureSkyForImage(Game& game)
{
	if (skybox != nullptr || !texture)
		return;
	if (skyRadiusVal <= 0.0f)
		skyRadiusVal = 4000.0f;
	skybox = new Skybox(game, "", skyRadiusVal);   // no panorama file: the painted sky draws on it
	game.entities.push_back(skybox);
}

bool HdrSkyImage(unsigned int& tex, int& v)
{
	tex = texture.id;
	v = version;
	return (bool)texture;
}

bool DrawHdrSky(const Renderer& renderer, float radius)
{
	if (!texture || renderer.camera.useOrthoCamera || Skybox::meshSkySphere == nullptr || !EnsureShader())
		return false;

	RenderDevice& device = Device();
	shader->UseShader();
	renderer.BindWorldCameraBlock();
	const unsigned int id = shader->GetID();
	device.BindTexture(0, texture);
	device.SetUniform(device.UniformLocation(ProgramHandle(id), "skyTexture"), 0);
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "p0"), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "p1"), glm::vec4(radius, 0.0f, 1.0f, 0.0f));
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "sun0"), glm::vec4(sun.toSun, std::sin(sun.radius)));
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "sun1"), glm::vec4(sun.radiance, sun.flatten));
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "sun2"), glm::vec4(sun.lowerTint, 0.0f));
	device.SetUniform(ShaderProgram::DrawUniformLocation(id, "sun3"), glm::vec4(sun.upperTint, 0.0f));

	// As the scene's panorama: opaque, writing depth, behind everything
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

void ReleaseHdrSky()
{
	if (texture)
		Device().DestroyTexture(texture);
	width = height = 0;
	delete shader;
	shader = nullptr;
	shaderFailed = false;
	sun = Sun();
}
