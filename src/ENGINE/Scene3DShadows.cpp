// Scene3D shadow passes: the sun's shadow map and point-light cube shadows.
// Split out of Scene3D.cpp (render-pass refactor, Phase 1 step 6).

#include "Scene3D.h"
#include "render/RenderDevice.h"
#include "Game.h"
#include "Renderer.h"
#include "Camera.h"
#include "Sprite.h"
#include "SpriteManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include "Skybox.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <map>
#include <tuple>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstddef>
#include "UniformBlocks.h"
#include "UniformBufferCache.h"
#include "ModelMaterials.h"
#include "RenderState.h"
#include "TransientBuffer.h"

#include "Scene3DInternal.h"
#include "render/ColorPipeline.h"
#include "render/ProgramEvents.h"
#include "render/RenderViews.h"
#include "globals.h"
#include <unordered_map>

using Scene3DInternal::ProgramHasBlock;

namespace
{
	// Shadow-map depth passes: depth test LESS with writes on. Blending is left as
	// it is (the frame's final compositing needs it; disabling it here once
	// blacked out the screen) and so is everything else.
	RenderState ShadowDepthState()
	{
		RenderState s = CurrentRenderState();
		s.depthTest = true;
		s.depthCompare = CompareOp::Less;
		s.depthWrite = true;
		return s;
	}

	// std140 mirror of the GLSL "ShadowPass" block (shaders/shadow_pass.glsl).
	struct ShadowPassBlockData
	{
		glm::mat4 viewProj;
		glm::vec3 lightPos;  float farPlane;
		float alphaCutoff;
		float pad[3];        // std140 rounds the block up to 16 bytes
	};
	static_assert(sizeof(ShadowPassBlockData) == 96, "ShadowPassBlockData must match shaders/shadow_pass.glsl");

	const UniformBlockMember kShadowPassMembers[] = {
		{ "viewProj", offsetof(ShadowPassBlockData, viewProj), false },
		{ "lightPos", offsetof(ShadowPassBlockData, lightPos), false },
		{ "farPlane", offsetof(ShadowPassBlockData, farPlane), false },
		{ "alphaCutoff", offsetof(ShadowPassBlockData, alphaCutoff), false },
	};

	// The sun's map plus 6 faces per point-light caster (up to 8): 64 buffers
	// keep a whole frame's targets resident.
	UniformBufferCache shadowPassBlocks(UniformBlock::ShadowPass, sizeof(ShadowPassBlockData), 64);

	// One shadow-map target's values, for the bound depth-pass program. `sun`
	// selects the legacy uniform name for the matrix (old shadow_depth copies
	// call it lightSpace; the cube pass called it viewProj).
	void ApplyShadowPass(unsigned int program, const glm::mat4& viewProj, const glm::vec3& lightPos,
		float farPlane, float alphaCutoff, bool sun)
	{
		if (ProgramHasBlock(program, "ShadowPass"))
		{
			CheckUniformBlockLayout(program, "ShadowPass", kShadowPassMembers,
				sizeof(kShadowPassMembers) / sizeof(kShadowPassMembers[0]), sizeof(ShadowPassBlockData));
			ShadowPassBlockData d = {};
			d.viewProj = viewProj;
			d.lightPos = lightPos;
			d.farPlane = farPlane;
			d.alphaCutoff = alphaCutoff;
			shadowPassBlocks.Bind(&d);
			return;
		}

		Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(program), sun ? "lightSpace" : "viewProj")), viewProj);
		Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(program), "alphaCutoff")), (float)(alphaCutoff));
		if (!sun)
		{
			Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(program), "lightPos")), lightPos);
			Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(program), "farPlane")), (float)(farPlane));
		}
	}

	// ------------------------------------------------ cascaded sun shadows
	// Phase 1.5 item 5 (docs/RENDERING_BACKEND_PLAN.md). Up to 4 maps in one
	// depth texture array, each covering a slice of the camera's view through
	// its bounding sphere: the sphere's size depends only on the slice, so it
	// doesn't breathe as the camera turns, and its centre is snapped to whole
	// shadow texels so the map doesn't swim as the camera moves. Each cascade
	// re-renders only when its matrix or the casters change.
	const int kMaxCascades = 4;
	const int kCascadeSize = 2048;

	struct CascadeSettings
	{
		bool loaded = false;
		int count = 4;
		float distance = 5000.0f;
		float softness = 1.0f;
		float minCasterHeight = 15.0f;   // `shadowMinCasterHeight` (every shadow map, not only the cascades)
	};
	CascadeSettings cascadeSettings;
	float sceneShadowDistance = -1.0f;   // .scene `shadowdistance`; < 0 = project default

	const CascadeSettings& Cascades()
	{
		if (!cascadeSettings.loaded)
		{
			cascadeSettings.loaded = true;
			auto config = ReadRendererConfig();
			try
			{
				if (config.count("shadowCascades") > 0)
					cascadeSettings.count = std::min(std::max(std::stoi(config["shadowCascades"]), 0), kMaxCascades);
				if (config.count("shadowDistance") > 0)
					cascadeSettings.distance = std::max(std::stof(config["shadowDistance"]), 100.0f);
				if (config.count("shadowSoftness") > 0)
					cascadeSettings.softness = std::max(std::stof(config["shadowSoftness"]), 0.0f);
				if (config.count("shadowMinCasterHeight") > 0)
					cascadeSettings.minCasterHeight = std::max(std::stof(config["shadowMinCasterHeight"]), 0.0f);
			}
			catch (...)
			{
				std::cout << "WARNING: renderer.dat shadow settings could not be read; using defaults" << std::endl;
			}
		}
		return cascadeSettings;
	}

	bool CascadesWanted()
	{
		return Cascades().count > 0;
	}

	// Profiling: KINJO_SHADOWS_EVERY_FRAME=1 redraws the shadow maps every frame,
	// as a scene with moving casters would, instead of reusing them.
	bool ShadowsEveryFrame()
	{
		static const bool on = []()
		{
			const char* v = std::getenv("KINJO_SHADOWS_EVERY_FRAME");
			return v != nullptr && v[0] == '1';
		}();
		return on;
	}

	// std140 mirror of the GLSL "Cascades" block (shaders/cascades.glsl).
	struct CascadesBlockData
	{
		glm::mat4 viewProj[kMaxCascades];
		glm::vec4 sphere[kMaxCascades];   // xyz centre, w radius^2
		glm::vec4 texelWorld;
		glm::vec4 depthBias;
		int count;
		float softness;
		float pad[2];
	};
	static_assert(sizeof(CascadesBlockData) == 368, "CascadesBlockData must match shaders/cascades.glsl");

	// Plain arrays report only element [0]; the vec4 array's 16-byte stride is
	// checked (isArray), the mat4 array's 64 is std140's and fixed.
	const UniformBlockMember kCascadesMembers[] = {
		{ "cascadeViewProj[0]", offsetof(CascadesBlockData, viewProj), false },
		{ "cascadeSphere[0]", offsetof(CascadesBlockData, sphere), true },
		{ "cascadeTexelWorld", offsetof(CascadesBlockData, texelWorld), false },
		{ "cascadeDepthBias", offsetof(CascadesBlockData, depthBias), false },
		{ "cascadeCount", offsetof(CascadesBlockData, count), false },
		{ "cascadeSoftness", offsetof(CascadesBlockData, softness), false },
	};
	UniformBufferCache cascadeBlocks(UniformBlock::Cascades, sizeof(CascadesBlockData), 4);

	TextureHandle cascadePlaceholder;      // 1x1 of the same kind: a shadow sampler must always see one

	// One set per split-screen view (render/RenderViews.h): each view's
	// cascades are fitted to its own camera and cached on their own.
	struct CascadeSet
	{
		TextureHandle texture;             // Depth24 array, kMaxCascades layers, hardware compare
		FramebufferHandle fbo;
		CascadesBlockData data = {};       // what the last render produced
		int thisFrame = 0;                 // 0 until this frame's cascades exist
		glm::mat4 rendered[kMaxCascades];  // per-layer cache: the matrix it was drawn with...
		double renderedSig[kMaxCascades] = {};   // ...and the casters it saw
		bool valid[kMaxCascades] = {};
	};
	CascadeSet cascadeSets[kMaxRenderViews];
	CascadeSet* cs = &cascadeSets[0];      // the view drawing now (Scene3DInternal::SetShadowView)

	void EnsureCascadeTargets()
	{
		if (cs->texture)
			return;
		RenderDevice& device = Device();
		TextureDesc desc;
		desc.type = TextureType::Tex2DArray;
		desc.format = TextureFormat::Depth24;
		desc.width = desc.height = kCascadeSize;
		desc.layers = kMaxCascades;
		desc.filter = TextureFilter::Linear;   // with compare: bilinear PCF per tap
		desc.wrap = TextureWrap::ClampToEdge;
		desc.depthCompare = true;
		cs->texture = device.CreateTexture(desc);
		cs->fbo = device.CreateFramebuffer();
		device.AttachTexture(cs->fbo, Attachment::Depth, cs->texture, TextureType::Tex2DArray, 0);
		device.SetDrawBuffers(cs->fbo, 0);   // depth only
		device.BindFramebuffer(FramebufferHandle());
		for (bool& v : cs->valid)
			v = false;
	}

	std::unordered_map<unsigned int, int> cascadeSamplerLoc;   // program -> "shadowCascades" location
}

void Scene3DInternal::ApplyShadowPass(unsigned int program, const glm::mat4& viewProj, const glm::vec3& lightPos,
	float farPlane, float alphaCutoff, bool sun)
{
	::ApplyShadowPass(program, viewProj, lightPos, farPlane, alphaCutoff, sun);
}

void Scene3DInternal::BindCascades(unsigned int program)
{
	CascadesBlockData d = cs->data;
	d.count = cs->thisFrame;
	cascadeBlocks.Bind(&d);
	if (program == 0)
		return;
	if (ProgramHasBlock(program, "Cascades"))
		CheckUniformBlockLayout(program, "Cascades", kCascadesMembers,
			sizeof(kCascadesMembers) / sizeof(kCascadesMembers[0]), sizeof(CascadesBlockData));

	// The sampler's unit is set even with no cascades: a shadow sampler left on
	// its default unit 0 would clash with the albedo sampler there.
	auto it = cascadeSamplerLoc.find(program);
	if (it == cascadeSamplerLoc.end())
	{
		AddProgramDeletedListener([](unsigned int p) { cascadeSamplerLoc.erase(p); });
		it = cascadeSamplerLoc.emplace(program, Device().UniformLocation(ProgramHandle(program), "shadowCascades")).first;
	}
	if (it->second >= 0)
	{
		Device().SetUniform(it->second, 11);
		if (cs->thisFrame > 0)
		{
			Device().BindTexture(11, cs->texture, TextureType::Tex2DArray);
		}
		else
		{
			// Not sampled (cascadeCount = 0), but a shadow sampler with no depth
			// texture behind it is undefined behaviour all the same.
			if (!cascadePlaceholder)
			{
				TextureDesc desc;
				desc.type = TextureType::Tex2DArray;
				desc.format = TextureFormat::Depth24;
				desc.width = desc.height = 1;
				desc.layers = 1;
				desc.filter = TextureFilter::Linear;
				desc.wrap = TextureWrap::ClampToEdge;
				desc.depthCompare = true;
				cascadePlaceholder = Device().CreateTexture(desc);
			}
			Device().BindTexture(11, cascadePlaceholder, TextureType::Tex2DArray);
		}
	}
}

void Scene3DInternal::SetSceneShadowDistance(float distance)
{
	sceneShadowDistance = (distance < 0.0f) ? -1.0f : std::max(distance, 100.0f);
}

float Scene3DInternal::SceneShadowDistance()
{
	return sceneShadowDistance;
}

float Scene3DInternal::ProjectShadowDistance()
{
	return Cascades().distance;
}

void Scene3DInternal::SetShadowView(int index)
{
	cs = &cascadeSets[(index >= 0 && index < kMaxRenderViews) ? index : 0];
}

void Scene3DInternal::ReloadShadowSettings()
{
	cascadeSettings = CascadeSettings();   // read again on next use
}

bool Scene3DInternal::CastsShadow(const Scene3DModel& m)
{
	const int choice = (m.material != nullptr) ? MaterialLibrary::Get().ShadowChoice(*m.material) : 0;
	if (choice != 0)
		return choice > 0;   // `shadow on` / `shadow off`
	// Models with real height cast; flat ground and slabs don't
	return std::fabs(m.aabbMax.y - m.aabbMin.y) >= Cascades().minCasterHeight;
}

namespace
{
	void RunCasterHook(unsigned int program);   // below, with SetShadowCasterHook
}

void Scene3D::RenderShadowCascades(Game& game, const Renderer& renderer, const glm::vec3& L, double casterSig)
{
	const CascadeSettings& cfg = Cascades();
	const Camera& cam = renderer.camera;
	const float nearD = std::max(cam.nearPlane, 1.0f);
	const float reach = (sceneShadowDistance > 0.0f) ? sceneShadowDistance : cfg.distance;
	const float farD = std::min(reach, cam.farPlane);
	const int count = cfg.count;
	if (count <= 0 || farD <= nearD * 2.0f || cam.farPlane <= cam.nearPlane)
		return;
	EnsureCascadeTargets();

	// The view frustum's corner rays, from the inverse view-projection (no
	// assumptions about the camera's axis conventions). View depth is linear
	// along each ray, so a slice's corners interpolate between near and far.
	const glm::mat4 invViewProj = glm::inverse(cam.projection * cam.CalculateViewMatrix());
	const glm::vec2 ndc[4] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
	glm::vec3 nearCorner[4], farCorner[4];
	for (int k = 0; k < 4; k++)
	{
		const glm::vec4 a = invViewProj * glm::vec4(ndc[k], -1.0f, 1.0f);
		const glm::vec4 b = invViewProj * glm::vec4(ndc[k], 1.0f, 1.0f);
		nearCorner[k] = glm::vec3(a) / a.w;
		farCorner[k] = glm::vec3(b) / b.w;
	}
	auto cornerAt = [&](int k, float depth)
	{
		const float t = (depth - cam.nearPlane) / (cam.farPlane - cam.nearPlane);
		return nearCorner[k] + (farCorner[k] - nearCorner[k]) * t;
	};

	// Split distances: a 3:1 blend of logarithmic (even texel density) and
	// linear (no tiny first slice), the usual "practical" scheme.
	float split[kMaxCascades + 1];
	split[0] = nearD;
	for (int i = 1; i <= count; i++)
	{
		const float s = (float)i / (float)count;
		const float logSplit = nearD * std::pow(farD / nearD, s);
		const float linSplit = nearD + (farD - nearD) * s;
		split[i] = 0.75f * logSplit + 0.25f * linSplit;
	}

	// The light's orientation (eye at the origin, looking along L).
	const glm::vec3 up = (std::fabs(L.y) > 0.97f) ? glm::vec3(0, 0, 1) : glm::vec3(0, -1, 0);
	const glm::mat4 lightRotation = glm::lookAt(glm::vec3(0.0f), L, up);

	RenderDevice& device = Device();
	bool bound = false;
	unsigned int id = shadowDepthShader->GetID();
	for (int i = 0; i < count; i++)
	{
		// Bounding sphere of the slice.
		glm::vec3 pts[8];
		glm::vec3 center(0.0f);
		for (int k = 0; k < 4; k++)
		{
			pts[k] = cornerAt(k, split[i]);
			pts[k + 4] = cornerAt(k, split[i + 1]);
		}
		for (const glm::vec3& p : pts)
			center += p;
		center /= 8.0f;
		float radius = 0.0f;
		for (const glm::vec3& p : pts)
			radius = std::max(radius, glm::length(p - center));
		radius = std::ceil(radius);   // stable size (no float jitter)

		// Light-space box around it, its centre snapped to whole texels.
		const float texel = 2.0f * radius / (float)kCascadeSize;
		glm::vec3 c = glm::vec3(lightRotation * glm::vec4(center, 1.0f));
		c.x = std::floor(c.x / texel) * texel;
		c.y = std::floor(c.y / texel) * texel;
		const float back = std::max(2.0f * radius, 3000.0f);   // casters toward the sun, outside the slice
		const glm::mat4 proj = glm::ortho(c.x - radius, c.x + radius, c.y - radius, c.y + radius,
			-c.z - radius - back, -c.z + radius);
		const glm::mat4 viewProj = proj * lightRotation;
		const float depthRange = 2.0f * radius + back;

		cs->data.viewProj[i] = viewProj;
		// Selection sphere a couple of texels inside the map's edge.
		const float selectRadius = radius - 2.0f * texel;
		cs->data.sphere[i] = glm::vec4(center, selectRadius * selectRadius);
		cs->data.texelWorld[i] = texel;
		cs->data.depthBias[i] = 0.5f * texel / depthRange;

		if (cs->valid[i] && cs->rendered[i] == viewProj && cs->renderedSig[i] == casterSig
			&& !ShadowsEveryFrame())
			continue;   // nothing moved in this cascade: keep its map
		cs->valid[i] = true;
		cs->rendered[i] = viewProj;
		cs->renderedSig[i] = casterSig;

		if (!bound)
		{
			device.SetViewport(0, 0, kCascadeSize, kCascadeSize);
			ApplyRenderState(ShadowDepthState());
			shadowDepthShader->UseShader();
			Device().SetUniform(Device().UniformLocation(ProgramHandle(id), "theTexture"), 0);
			bound = true;
		}
		device.AttachTexture(cs->fbo, Attachment::Depth, cs->texture, TextureType::Tex2DArray, i);
		device.BindFramebuffer(cs->fbo);
		device.Clear(false, true);
		ApplyShadowPass(id, viewProj, glm::vec3(0.0f), 1.0f, 0.5f, true);
		DrawShadowCasters(renderer, id, center, radius, L);
		if (Scene3DInternal::GpuDrivenFrame())
		{
			// GPU-driven casters (Scene3DGpuDriven.cpp), culled to this cascade.
			const unsigned int gpu = Scene3DInternal::GpuShadowProgram(false);
			device.UseProgram(ProgramHandle(gpu));
			ApplyShadowPass(gpu, viewProj, glm::vec3(0.0f), 1.0f, 0.5f, true);
			DrawGpuDepthView(viewProj, gpu);
			shadowDepthShader->UseShader();
		}
	}

	cs->data.count = count;
	cs->data.softness = cfg.softness;
	cs->thisFrame = count;
	if (bound)
	{
		device.BindFramebuffer(FramebufferHandle());
		device.SetViewport(0, 0, ViewTargetWidth(game), ViewTargetHeight(game));
	}
}

// Sun-shadow casters for one cascade: models with real height (not flat
// ground, not water) and visible characters, skipping anything that can't
// shadow the cascade's sphere - off to the side of it as seen from the sun, or
// entirely beyond it along the light.
void Scene3D::DrawShadowCasters(const Renderer& renderer, unsigned int program, const glm::vec3& center,
	float radius, const glm::vec3& L)
{
	auto canShadow = [&](const glm::vec3& c, float r)
	{
		const glm::vec3 v = c - center;
		const float along = glm::dot(v, L);
		if (along > radius + r)
			return false;   // past the sphere: it would only shadow things farther on
		const glm::vec3 across = v - along * L;
		return glm::dot(across, across) <= (radius + r) * (radius + r);
	};

	// (Models the GPU-driven path draws are drawn after this, in RenderShadowCascades.)
	for (Scene3DModel* m : Scene3DInternal::CpuCasterModels(models))
	{
		if (m == nullptr || !m->loaded || m->texture == nullptr || m->IsWater() || m->guardHidden)
			continue;
		if (!Scene3DInternal::CastsShadow(*m))
			continue;
		const glm::vec3 c = (m->aabbMin + m->aabbMax) * 0.5f;
		if (!canShadow(c, glm::length(m->aabbMax - m->aabbMin) * 0.5f))
			continue;
		glm::mat4 model(1.0f);
		model = glm::translate(model, m->position);
		model = glm::rotate(model, glm::radians(m->yawDeg), glm::vec3(0, -1, 0));
		model = glm::rotate(model, glm::radians(m->pitchDeg), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(m->rollDeg), glm::vec3(0, 0, 1));
		model = glm::scale(model, m->EffectiveScale());
		const std::vector<Mesh*>* meshes = Scene3DInternal::LevelMeshes(*m, model);   // its level of detail
		if (meshes == nullptr)
			continue;
		Device().SetUniform(ShaderProgram::DrawUniformLocation(program, "model"), model);
		Scene3DInternal::DrawMeshesForDepth(*meshes, m->texture);
	}

	// Characters: the quad oriented exactly like the visible billboard (yaw-only,
	// facing the camera), as in the single-map pass.
	const glm::vec3 worldUp(0.0f, -1.0f, 0.0f);
	for (Character3D* ch : characters)
	{
		if (!CharVisible(ch))
			continue;
		if (ch == nullptr || ch->quad == nullptr || ch->bodyTex == nullptr)
			continue;
		if (!canShadow(ch->position + worldUp * (ch->worldHeight * 0.5f), ch->worldHeight * 0.6f))
			continue;
		glm::vec3 toCam = renderer.camera.position - ch->position;
		toCam.y = 0.0f;
		if (glm::length(toCam) < 1e-4f) toCam = glm::vec3(0, 0, 1);
		toCam = glm::normalize(toCam);
		const glm::vec3 right = glm::normalize(glm::cross(toCam, worldUp));
		const float aspect = (ch->bodyTex->GetHeight() > 0)
			? (float)ch->bodyTex->GetWidth() / (float)ch->bodyTex->GetHeight() : 0.5f;
		const float width = ch->worldHeight * aspect;
		glm::mat4 model(1.0f);
		model[0] = glm::vec4(right * width, 0.0f);
		model[1] = glm::vec4(worldUp * ch->worldHeight, 0.0f);
		model[2] = glm::vec4(toCam, 0.0f);
		model[3] = glm::vec4(ch->position, 1.0f);
		Device().SetUniform(ShaderProgram::DrawUniformLocation(program, "model"), model);
		ch->bodyTex->UseTexture();
		ch->quad->RenderMesh(0);
		if (ch->headTex != nullptr)
		{
			ch->headTex->UseTexture();
			ch->quad->RenderMesh(0);
		}
	}

	RunCasterHook(program);   // a game's own moving geometry (SetShadowCasterHook)
}

void Scene3D::EnsureShadowMap()
{
	if (shadowFBO != 0)
		return;
	RenderDevice& device = Device();
	const FramebufferHandle fb = device.CreateFramebuffer();
	shadowFBO = fb.id;

	TextureDesc desc;
	desc.format = TextureFormat::Depth24;
	desc.width = desc.height = shadowMapSize;
	desc.filter = TextureFilter::Linear;
	desc.wrap = TextureWrap::ClampToBorder;
	desc.borderColor = glm::vec4(1.0f);   // outside frustum = far = lit
	const TextureHandle depth = device.CreateTexture(desc);
	shadowDepthTex = depth.id;

	device.AttachTexture(fb, Attachment::Depth, depth);
	device.SetDrawBuffers(fb, 0);         // depth only
	device.BindFramebuffer(FramebufferHandle());
}

namespace
{
	// SetShadowCasterHook: a game's own moving geometry in the sun's shadow.
	std::function<void()> casterHookDraw;
	std::function<double()> casterHookSignature;
	unsigned int casterHookProgram = 0;   // the depth program while the hook draws

	void RunCasterHook(unsigned int program)
	{
		if (!casterHookDraw)
			return;
		casterHookProgram = program;
		ModelWhiteTexture()->UseTexture();   // opaque: nothing cut out
		casterHookDraw();
		casterHookProgram = 0;
	}
}

void Scene3D::SetShadowCasterHook(std::function<void()> draw, std::function<double()> signature)
{
	casterHookDraw = std::move(draw);
	casterHookSignature = std::move(signature);
}

void Scene3D::DrawShadowMesh(Mesh* mesh, const glm::mat4& model) const
{
	if (casterHookProgram == 0 || mesh == nullptr)
		return;
	Device().SetUniform(ShaderProgram::DrawUniformLocation(casterHookProgram, "model"), model);
	mesh->RenderMesh(0);
}

void Scene3D::RenderShadowDepth(Game& game, const Renderer& renderer)
{
#ifdef USE_ASSIMP
	shadowActive = false;
	cs->thisFrame = 0;
	if (!active || !shadowsEnabled || renderer.camera.useOrthoCamera)
		return;
	// Casters' levels of detail are the ones this view's camera sees
	Scene3DInternal::SetLevelCamera(renderer.camera.position, renderer.camera.projection[1][1]);
	if (dirLight.diffuse <= 0.02f)   // no sun (night / point-lit room) -> no shadows
		return;

	glm::vec3 L = dirLight.dir;
	if (glm::length(L) < 1e-4f) return;
	L = glm::normalize(L);   // direction the sunlight travels (into the scene)

	if (shadowDepthShader == nullptr)
		return;

	// --- static caching (mirrors the point-shadow cache): only re-render the
	// shadow map when the sun direction or a caster (a height-having model or a
	// character) actually moved. In a still, time-frozen scene the depth pass runs
	// once then is skipped every subsequent frame. ---
	double sig = L.x * 101.1 + L.y * 211.3 + L.z * 307.7;
	for (Scene3DModel* m : models)
	{
		if (m == nullptr || !m->loaded || m->texture == nullptr || m->IsWater() || m->guardHidden)
			continue;   // guardHidden in the sig -> shadow re-renders when it appears/hides
		if (!Scene3DInternal::CastsShadow(*m))
			continue;
		glm::vec3 s = m->EffectiveScale();
		sig += m->position.x * 1.1 + m->position.y * 2.3 + m->position.z * 3.7
			+ m->yawDeg * 0.017 + m->pitchDeg * 0.013 + m->rollDeg * 0.011
			+ (s.x + s.y + s.z) * 5.3;
	}
	for (Character3D* ch : characters)
		if (ch != nullptr)
			sig += ch->position.x * 1.3 + ch->position.y * 2.1 + ch->position.z * 4.3;
	// Character shadows are cast from camera-facing billboards (see below), so the
	// shadow map must refresh as the camera orbits. Fold the camera position into
	// the signature - but only when there are characters, so a character-free scene
	// still caches its (camera-independent) prop shadows across camera moves.
	if (!characters.empty())
		sig += renderer.camera.position.x * 0.71 + renderer.camera.position.y * 0.93
		     + renderer.camera.position.z * 1.29;
	if (casterHookSignature)
		sig += casterHookSignature() * 1.37;   // a game's own moving casters

	// Cascaded maps when the scene's model shader reads them (the engine's
	// scene3d.frag); an older game copy keeps the single map below.
	if (CascadesWanted() && shader != nullptr && ProgramHasBlock(shader->GetID(), "Cascades"))
	{
		RenderShadowCascades(game, renderer, L, sig);
		return;
	}

	EnsureShadowMap();
	if (shadowEverRendered && sig == shadowSig)
	{
		shadowActive = true;   // reuse the cached shadow map; no depth work this frame
		return;
	}
	shadowSig = sig;
	shadowEverRendered = true;

	// Orthographic light frustum covering the scene (centred on the origin, a
	// little below ground so it spans the props' height; up is -Y).
	const float R = 3200.0f;                 // half-size of the covered area
	glm::vec3 center(0.0f, -150.0f, 0.0f);
	glm::vec3 eye = center - L * (R * 1.6f);
	glm::vec3 up = (std::fabs(L.y) > 0.97f) ? glm::vec3(0, 0, 1) : glm::vec3(0, -1, 0);
	glm::mat4 lightView = glm::lookAt(eye, center, up);
	glm::mat4 lightProj = glm::ortho(-R, R, -R, R, 1.0f, R * 3.5f);
	lightSpaceMatrix = lightProj * lightView;

	RenderDevice& device = Device();
	device.SetViewport(0, 0, shadowMapSize, shadowMapSize);
	device.BindFramebuffer(FramebufferHandle(shadowFBO));
	// Depth writes only happen with the depth test ENABLED; at the start of a
	// frame it may be off (left by the 2D/GUI pass), which would leave the shadow
	// map empty (no shadows at all). Force just the depth state here - do NOT
	// touch blending (the frame's final compositing needs it; disabling it here
	// blacked out the screen).
	ApplyRenderState(ShadowDepthState());
	device.Clear(false, true);

	shadowDepthShader->UseShader();
	unsigned int id = shadowDepthShader->GetID();
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));
	ApplyShadowPass(id, lightSpaceMatrix, glm::vec3(0.0f), 1.0f, 0.5f, true);

	// Casters: models with real vertical extent (skips flat ground/sidewalks/
	// lakebed) and not the water surface.
	for (Scene3DModel* m : models)
	{
		if (m == nullptr || !m->loaded || m->texture == nullptr || m->IsWater() || m->guardHidden)
			continue;
		if (!Scene3DInternal::CastsShadow(*m))
			continue;
		glm::mat4 model(1.0f);
		model = glm::translate(model, m->position);
		model = glm::rotate(model, glm::radians(m->yawDeg), glm::vec3(0, -1, 0));
		model = glm::rotate(model, glm::radians(m->pitchDeg), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(m->rollDeg), glm::vec3(0, 0, 1));
		model = glm::scale(model, m->EffectiveScale());
		const std::vector<Mesh*>* meshes = Scene3DInternal::LevelMeshes(*m, model);   // its level of detail
		if (meshes == nullptr)
			continue;
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), model);
		Scene3DInternal::DrawMeshesForDepth(*meshes, m->texture);
	}

	// Character casters: orient the shadow quad EXACTLY like the visible billboard
	// (yaw-only, facing the camera - see Character3D::DrawQuad), not facing the sun.
	// A billboard is a flat cutout; casting its shadow from the same orientation the
	// player sees makes the shadow rotate in sync with the sprite as the camera
	// orbits. (Facing the sun instead kept the shadow locked while the sprite turned,
	// which looked wrong.)
	glm::vec3 worldUp(0.0f, -1.0f, 0.0f);
	for (Character3D* ch : characters)
	{
		if (!CharVisible(ch))   // hidden (backdrop or non-solo): cast no shadow
			continue;
		if (ch == nullptr || ch->quad == nullptr || ch->bodyTex == nullptr)
			continue;
		glm::vec3 toCam = renderer.camera.position - ch->position;
		toCam.y = 0.0f;  // yaw-only, matches DrawQuad
		if (glm::length(toCam) < 1e-4f) toCam = glm::vec3(0, 0, 1);
		toCam = glm::normalize(toCam);
		glm::vec3 right = glm::normalize(glm::cross(toCam, worldUp));
		float aspect = (ch->bodyTex->GetHeight() > 0)
			? (float)ch->bodyTex->GetWidth() / (float)ch->bodyTex->GetHeight() : 0.5f;
		float width = ch->worldHeight * aspect;
		glm::mat4 model(1.0f);
		model[0] = glm::vec4(right * width, 0.0f);
		model[1] = glm::vec4(worldUp * ch->worldHeight, 0.0f);
		model[2] = glm::vec4(toCam, 0.0f);
		model[3] = glm::vec4(ch->position, 1.0f);
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), model);
		ch->bodyTex->UseTexture();
		ch->quad->RenderMesh(0);
		if (ch->headTex != nullptr)
		{
			ch->headTex->UseTexture();
			ch->quad->RenderMesh(0);
		}
	}

	RunCasterHook(id);   // a game's own moving geometry (SetShadowCasterHook)

	// Restore the default framebuffer + full viewport; Game::Render rebinds the
	// main scene framebuffer next.
	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, ViewTargetWidth(game), ViewTargetHeight(game));
	shadowActive = true;
#endif
}

void Scene3D::EnsurePointShadowMaps()
{
	RenderDevice& device = Device();
	if (pointShadowFBO == 0)
		pointShadowFBO = device.CreateFramebuffer().id;

	TextureDesc desc;
	desc.format = TextureFormat::Depth24;
	desc.width = desc.height = pointShadowSize;
	desc.filter = TextureFilter::Nearest;
	desc.wrap = TextureWrap::ClampToEdge;

	if (device.SupportsCubeMapArrays())
	{
		// One cube-map ARRAY: kMaxPointShadows cubes (6 faces each) as layers.
		if (pointShadowArrayTex == 0)
		{
			desc.type = TextureType::CubeArray;
			desc.layers = kMaxPointShadows;
			pointShadowArrayTex = device.CreateTexture(desc).id;
		}
	}
	else
	{
		desc.type = TextureType::Cube;
		for (int c = 0; c < kMaxPointShadowsFallback; c++)
		{
			if (pointShadowCubes[c] != 0) continue;
			pointShadowCubes[c] = device.CreateTexture(desc).id;
		}
	}
	const FramebufferHandle fb(pointShadowFBO);
	device.SetDrawBuffers(fb, 0);         // depth only
	device.BindFramebuffer(FramebufferHandle());
}

void Scene3D::RenderPointShadowDepth(Game& game, const Renderer& renderer)
{
#ifdef USE_ASSIMP
	pointShadowActive = false;
	if (!active || !pointShadowsEnabled || renderer.camera.useOrthoCamera)
		return;
	// Casters' levels of detail are the ones the camera sees
	Scene3DInternal::SetLevelCamera(renderer.camera.position, renderer.camera.projection[1][1]);
	// Only indoors: an outdoor scene with a sun uses the directional shadow map.
	if (dirLight.diffuse > 0.02f)
		return;

	// A GL 4.x desktop context uses a cube-map ARRAY (up to kMaxPointShadows
	// casters, dynamic layer index); the 3.3/web fallback uses separate cubes
	// constant-indexed in the shader, so it caps at kMaxPointShadowsFallback.
	RenderDevice& device = Device();
	const bool useArray = device.SupportsCubeMapArrays();
	const int maxCasters = useArray ? kMaxPointShadows : kMaxPointShadowsFallback;

	// Choose the casters: a specific named light (only that one), else AUTO = all
	// enabled point lights, strongest first, up to maxCasters.
	std::vector<int> casters;
	if (!shadowCasterLight.empty())
	{
		for (size_t i = 0; i < pointLights.size(); i++)
			if (pointLights[i].on && pointLights[i].name == shadowCasterLight)
			{ casters.push_back((int)i); break; }
	}
	else
	{
		std::vector<int> on;
		for (size_t i = 0; i < pointLights.size(); i++)
			if (pointLights[i].on && !pointLights[i].guardHidden) on.push_back((int)i);
		std::sort(on.begin(), on.end(), [&](int a, int b) {
			return pointLights[a].intensity * pointLights[a].range
			     > pointLights[b].intensity * pointLights[b].range;
		});
		for (int idx : on)
		{
			if ((int)casters.size() >= maxCasters) break;
			casters.push_back(idx);
		}
	}
	if (casters.empty()) return;

	EnsurePointShadowMaps();
	if (pointShadowShader == nullptr) return;

	pointShadowCount = (int)casters.size();
	for (int s = 0; s < pointShadowCount; s++)
	{
		pointShadowPositions[s] = pointLights[casters[s]].pos;
		pointShadowFars[s] = pointLights[casters[s]].range;
	}

	// --- static caching: only re-render the cubes when something that affects
	// them has moved (caster lights, props, or characters). In a still room the
	// depth passes run once then are skipped every subsequent frame. ---
	double sig = pointShadowCount * 1000003.0;
	for (int s = 0; s < pointShadowCount; s++)
		sig += (casters[s] + 1) * 7919.0
			+ pointShadowPositions[s].x * 1.1 + pointShadowPositions[s].y * 2.3 + pointShadowPositions[s].z * 3.7;
	for (Scene3DModel* m : models)
		if (m && m->loaded && !m->IsWater() && !m->guardHidden)
		{
			// Turning or resizing a model changes its shadow too (a door swinging).
			const glm::vec3 s = m->EffectiveScale();
			sig += m->position.x * 1.7 + m->position.y * 2.9 + m->position.z * 3.1
				+ m->yawDeg * 0.019 + m->pitchDeg * 0.023 + m->rollDeg * 0.029
				+ (s.x * 1.3 + s.y * 1.7 + s.z * 2.3);
		}
	for (Character3D* ch : characters)
		if (ch)
			sig += ch->position.x * 1.3 + ch->position.y * 2.1 + ch->position.z * 4.3;
	// Character shadows cast from camera-facing billboards (see below) -> refresh the
	// cubes as the camera orbits, but only when characters are present (a scene with
	// only props keeps caching its camera-independent shadows).
	if (!characters.empty())
		sig += renderer.camera.position.x * 0.71 + renderer.camera.position.y * 0.93
		     + renderer.camera.position.z * 1.29;

	if (pointShadowEverRendered && sig == pointShadowSig && !ShadowsEveryFrame())
	{
		pointShadowActive = true;   // reuse the cached cubes; no depth work this frame
		return;
	}
	pointShadowSig = sig;
	pointShadowEverRendered = true;

	pointShadowShader->UseShader();
	unsigned int id = pointShadowShader->GetID();
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));

	const FramebufferHandle fb(pointShadowFBO);
	device.SetViewport(0, 0, pointShadowSize, pointShadowSize);
	device.BindFramebuffer(fb);
	ApplyRenderState(ShadowDepthState());

	const glm::vec3 worldUp(0, -1, 0);
	for (int s = 0; s < pointShadowCount; s++)
	{
		const glm::vec3 P = pointShadowPositions[s];
		const float farP = pointShadowFars[s];
		glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 5.0f, farP);
		glm::mat4 views[6] = {
			glm::lookAt(P, P + glm::vec3( 1, 0, 0), glm::vec3(0, -1,  0)),
			glm::lookAt(P, P + glm::vec3(-1, 0, 0), glm::vec3(0, -1,  0)),
			glm::lookAt(P, P + glm::vec3( 0, 1, 0), glm::vec3(0,  0,  1)),
			glm::lookAt(P, P + glm::vec3( 0,-1, 0), glm::vec3(0,  0, -1)),
			glm::lookAt(P, P + glm::vec3( 0, 0, 1), glm::vec3(0, -1,  0)),
			glm::lookAt(P, P + glm::vec3( 0, 0,-1), glm::vec3(0, -1,  0)),
		};
		for (int face = 0; face < 6; face++)
		{
			if (useArray)
				// cube s occupies array layers [s*6 .. s*6+5]; layer index = s in the shader
				device.AttachTexture(fb, Attachment::Depth, TextureHandle(pointShadowArrayTex),
					TextureType::CubeArray, s * 6 + face);
			else
				device.AttachTexture(fb, Attachment::Depth, TextureHandle(pointShadowCubes[s]),
					TextureType::Cube, face);
			device.Clear(false, true);
			glm::mat4 vp = proj * views[face];
			ApplyShadowPass(id, vp, P, farP, 0.5f, false);

			// Models with real height, RANGE-CULLED: skip anything whose bounding
			// sphere is entirely beyond this light's reach (its shadow can't land
			// on a lit surface).
			// (Models the GPU-driven path draws are drawn below.)
			for (Scene3DModel* m : Scene3DInternal::CpuCasterModels(models))
			{
				if (m == nullptr || !m->loaded || m->texture == nullptr || m->IsWater() || m->guardHidden)
					continue;
				if (!Scene3DInternal::CastsShadow(*m))
					continue;
				glm::vec3 c = (m->aabbMin + m->aabbMax) * 0.5f;
				float r = glm::length(m->aabbMax - m->aabbMin) * 0.5f;
				if (glm::length(c - P) - r > farP)
					continue;
				glm::mat4 model(1.0f);
				model = glm::translate(model, m->position);
				model = glm::rotate(model, glm::radians(m->yawDeg), glm::vec3(0, -1, 0));
				model = glm::rotate(model, glm::radians(m->pitchDeg), glm::vec3(1, 0, 0));
				model = glm::rotate(model, glm::radians(m->rollDeg), glm::vec3(0, 0, 1));
				model = glm::scale(model, m->EffectiveScale());
				const std::vector<Mesh*>* meshes = Scene3DInternal::LevelMeshes(*m, model);   // its level of detail
				if (meshes == nullptr)
					continue;
				Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), model);
				Scene3DInternal::DrawMeshesForDepth(*meshes, m->texture);
			}

			// Characters: orient the shadow quad like the visible billboard (yaw-only,
			// facing the camera - see Character3D::DrawQuad), so the cast shadow tracks
			// the sprite as the camera orbits. Range-culled by the light's reach.
			for (Character3D* ch : characters)
			{
				if (!CharVisible(ch))   // hidden (backdrop or non-solo): cast no shadow
					continue;
				if (ch == nullptr || ch->quad == nullptr || ch->bodyTex == nullptr)
					continue;
				if (glm::length(ch->position - P) - ch->worldHeight * 0.5f > farP)
					continue;
				glm::vec3 toCam = renderer.camera.position - ch->position; toCam.y = 0.0f;
				if (glm::length(toCam) < 1e-4f) toCam = glm::vec3(0, 0, 1);
				toCam = glm::normalize(toCam);
				glm::vec3 right = glm::normalize(glm::cross(toCam, worldUp));
				float aspect = (ch->bodyTex->GetHeight() > 0)
					? (float)ch->bodyTex->GetWidth() / (float)ch->bodyTex->GetHeight() : 0.5f;
				float width = ch->worldHeight * aspect;
				glm::mat4 model(1.0f);
				model[0] = glm::vec4(right * width, 0.0f);
				model[1] = glm::vec4(worldUp * ch->worldHeight, 0.0f);
				model[2] = glm::vec4(toCam, 0.0f);
				model[3] = glm::vec4(ch->position, 1.0f);
				Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), model);
				ch->bodyTex->UseTexture();
				ch->quad->RenderMesh(0);
				if (ch->headTex != nullptr)
				{
					ch->headTex->UseTexture();
					ch->quad->RenderMesh(0);
				}
			}

			if (Scene3DInternal::GpuDrivenFrame())
			{
				// GPU-driven casters (Scene3DGpuDriven.cpp), culled to this face.
				const unsigned int gpu = Scene3DInternal::GpuShadowProgram(true);
				device.UseProgram(ProgramHandle(gpu));
				ApplyShadowPass(gpu, vp, P, farP, 0.5f, false);
				DrawGpuDepthView(vp, gpu);
				pointShadowShader->UseShader();
			}
		}
	}

	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, ViewTargetWidth(game), ViewTargetHeight(game));
	pointShadowActive = true;
#endif
}
