// Renderer side of the engine's shared uniform blocks (see UniformBlocks.h,
// UniformBufferCache.h and shaders/camera.glsl). State is file-local, like
// RendererOverlay.cpp, so Renderer's layout is unchanged.

#include "leak_check.h"
#include "Renderer.h"
#include "UniformBlocks.h"
#include "UniformBufferCache.h"
#include "PointLight.h"
#include "render/RenderDevice.h"
#include <cstddef>

namespace
{
	// std140 layout of the GLSL "Camera" block: two column-major mat4s.
	struct CameraBlockData
	{
		glm::mat4 view;
		glm::mat4 projection;
	};

	// A frame typically uses 2-3 distinct cameras (world, GUI text, GUI sprites
	// that keep the world projection); the editor minimap adds one more.
	UniformBufferCache cameraBlocks(UniformBlock::Camera, sizeof(CameraBlockData), 4);

	// std140 mirror of the GLSL "SpriteLights" block (shaders/sprite_lights.glsl):
	// struct Light rounds to 32 bytes and struct PointLight to 64.
	struct SpriteLightData
	{
		glm::vec3 color;           //  0  (Light base)
		float ambientIntensity;    // 12
		float diffuseIntensity;    // 16
		float pad0[3];             // 20  Light rounds up to 32
		glm::vec3 position;        // 32
		float constant;            // 44
		float linear;              // 48
		float exponent;            // 52
		float pad1[2];             // 56  PointLight rounds up to 64
	};
	struct SpriteLightsBlockData
	{
		int pointLightCount;
		int pad[3];                // the struct array starts on a 16-byte boundary
		SpriteLightData pointLights[MAX_POINT_LIGHTS];
	};
	static_assert(sizeof(SpriteLightData) == 64, "std140 PointLight stride");
	static_assert(sizeof(SpriteLightsBlockData) == 16 + 64 * MAX_POINT_LIGHTS, "SpriteLights block size");

	const UniformBlockMember kSpriteLightMembers[] = {
		{ "pointLightCount", offsetof(SpriteLightsBlockData, pointLightCount), false },
		{ "pointLights[0].base.color", offsetof(SpriteLightsBlockData, pointLights) + offsetof(SpriteLightData, color), false },
		{ "pointLights[0].base.ambientIntensity", offsetof(SpriteLightsBlockData, pointLights) + offsetof(SpriteLightData, ambientIntensity), false },
		{ "pointLights[0].base.diffuseIntensity", offsetof(SpriteLightsBlockData, pointLights) + offsetof(SpriteLightData, diffuseIntensity), false },
		{ "pointLights[0].position", offsetof(SpriteLightsBlockData, pointLights) + offsetof(SpriteLightData, position), false },
		{ "pointLights[0].constant", offsetof(SpriteLightsBlockData, pointLights) + offsetof(SpriteLightData, constant), false },
		{ "pointLights[0].linear", offsetof(SpriteLightsBlockData, pointLights) + offsetof(SpriteLightData, linear), false },
		{ "pointLights[0].exponent", offsetof(SpriteLightsBlockData, pointLights) + offsetof(SpriteLightData, exponent), false },
		// element 1 pins the 64-byte struct stride
		{ "pointLights[1].base.color", offsetof(SpriteLightsBlockData, pointLights) + sizeof(SpriteLightData), false },
	};

	// The lights change rarely (a flickering torch at most), so two buffers do.
	UniformBufferCache spriteLightBlocks(UniformBlock::SpriteLights, sizeof(SpriteLightsBlockData), 2);
}

bool Renderer::BindSpriteLightsBlock(const ShaderProgram& shader) const
{
	const unsigned int program = shader.GetID();
	if (!Device().HasUniformBlock(ProgramHandle(program), "SpriteLights"))
		return false;
	CheckUniformBlockLayout(program, "SpriteLights", kSpriteLightMembers,
		sizeof(kSpriteLightMembers) / sizeof(kSpriteLightMembers[0]), sizeof(SpriteLightsBlockData));

	if (pointLightCount > MAX_POINT_LIGHTS)
		pointLightCount = MAX_POINT_LIGHTS;

	SpriteLightsBlockData data = {};
	data.pointLightCount = (int)pointLightCount;
	for (unsigned int i = 0; i < pointLightCount; i++)
	{
		SpriteLightData& d = data.pointLights[i];
		const PointLight* p = pointLights[i];
		if (p == nullptr)
		{
			// Empty slot inside the active count: a black light with constant
			// attenuation 1 (a zero constant divides by zero in the shader).
			d.constant = 1.0f;
			continue;
		}
		d.color = p->color;
		d.ambientIntensity = p->ambientIntensity;
		d.diffuseIntensity = p->diffuseIntensity;
		d.position = p->position;
		d.constant = p->constant;
		d.linear = p->linear;
		d.exponent = p->exponent;
	}
	spriteLightBlocks.Bind(&data);
	return true;
}

void Renderer::BindCameraBlock(const glm::mat4& view, const glm::mat4& projection) const
{
	const CameraBlockData data{ view, projection };
	cameraBlocks.Bind(&data);
}

void Renderer::BindWorldCameraBlock() const
{
	BindCameraBlock(camera.CalculateViewMatrix(), camera.projection);
}

void Renderer::ReleaseUniformBlocks()
{
	// Every block's buffers (Camera here, Scene/Material in Scene3D.cpp).
	UniformBufferCache::ReleaseAll();
}
