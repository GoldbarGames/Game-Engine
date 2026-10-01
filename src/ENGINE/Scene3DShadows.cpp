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
#include "RenderState.h"
#include "TransientBuffer.h"

#include "Scene3DInternal.h"

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

void Scene3D::RenderShadowDepth(Game& game, const Renderer& renderer)
{
#ifdef USE_ASSIMP
	shadowActive = false;
	if (!active || !shadowsEnabled || renderer.camera.useOrthoCamera)
		return;
	if (dirLight.diffuse <= 0.02f)   // no sun (night / point-lit room) -> no shadows
		return;

	glm::vec3 L = dirLight.dir;
	if (glm::length(L) < 1e-4f) return;
	L = glm::normalize(L);   // direction the sunlight travels (into the scene)

	EnsureShadowMap();
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
		if (std::fabs(m->aabbMax.y - m->aabbMin.y) < 15.0f)
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
		if (std::fabs(m->aabbMax.y - m->aabbMin.y) < 15.0f)
			continue;
		glm::mat4 model(1.0f);
		model = glm::translate(model, m->position);
		model = glm::rotate(model, glm::radians(m->yawDeg), glm::vec3(0, -1, 0));
		model = glm::rotate(model, glm::radians(m->pitchDeg), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(m->rollDeg), glm::vec3(0, 0, 1));
		model = glm::scale(model, m->EffectiveScale());
		Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), model);
		m->texture->UseTexture();
		for (Mesh* mesh : m->model3D.meshList)
			mesh->RenderMesh(0);
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

	// Restore the default framebuffer + full viewport; Game::Render rebinds the
	// main scene framebuffer next.
	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, game.screenWidth, game.screenHeight);
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
			sig += m->position.x * 1.7 + m->position.y * 2.9 + m->position.z * 3.1;
	for (Character3D* ch : characters)
		if (ch)
			sig += ch->position.x * 1.3 + ch->position.y * 2.1 + ch->position.z * 4.3;
	// Character shadows cast from camera-facing billboards (see below) -> refresh the
	// cubes as the camera orbits, but only when characters are present (a scene with
	// only props keeps caching its camera-independent shadows).
	if (!characters.empty())
		sig += renderer.camera.position.x * 0.71 + renderer.camera.position.y * 0.93
		     + renderer.camera.position.z * 1.29;

	if (pointShadowEverRendered && sig == pointShadowSig)
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
			for (Scene3DModel* m : models)
			{
				if (m == nullptr || !m->loaded || m->texture == nullptr || m->IsWater() || m->guardHidden)
					continue;
				if (std::fabs(m->aabbMax.y - m->aabbMin.y) < 15.0f)
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
				Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), model);
				m->texture->UseTexture();
				for (Mesh* mesh : m->model3D.meshList)
					mesh->RenderMesh(0);
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
		}
	}

	device.BindFramebuffer(FramebufferHandle());
	device.SetViewport(0, 0, game.screenWidth, game.screenHeight);
	pointShadowActive = true;
#endif
}
