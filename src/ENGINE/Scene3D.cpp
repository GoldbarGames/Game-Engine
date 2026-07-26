#include "Scene3D.h"
#include "Game.h"
#include "Renderer.h"
#include "Camera.h"
#include "Sprite.h"
#include "SpriteManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include "Skybox.h"
#include "opengl_includes.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// ---------------------------------------------------------------- models

Scene3DModel::Scene3DModel(const glm::vec3& pos) : Entity(pos)
{
	etype = "scene3dmodel";
	name = "scene3dmodel";
	layer = DrawingLayer::BACK;  // behind VN sprites and the textbox
	drawOrder = -100;
}

void Scene3DModel::Update(Game& game)
{
	Entity::Update(game);
}

void Scene3DModel::Render(const Renderer& renderer)
{
	// Transparent models (material opacity < 1) are drawn later, back-to-front,
	// in Scene3D::RenderTransparentModels - skip them in the normal opaque pass.
	if (material != nullptr && material->IsTransparent())
		return;
	DrawGeometry(renderer);
}

void Scene3DModel::DrawGeometry(const Renderer& renderer)
{
#ifdef USE_ASSIMP
	if (!loaded || shader == nullptr || texture == nullptr
		|| renderer.camera.useOrthoCamera)
	{
		return;
	}

	glm::mat4 model(1.0f);
	model = glm::translate(model, position);
	// Yaw about vertical (-Y), then pitch about X, then roll about Z.
	model = glm::rotate(model, glm::radians(yawDeg), glm::vec3(0, -1, 0));
	model = glm::rotate(model, glm::radians(pitchDeg), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians(rollDeg), glm::vec3(0, 0, 1));
	model = glm::scale(model, EffectiveScale());

	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

	Scene3D& scene = Scene3D::Get();
	const SceneMaterial& mat = material ? *material : MaterialLibrary::Get().Default();

	shader->UseShader();
	GLuint id = shader->GetID();
	glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(glGetUniformLocation(id, "view"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.CalculateViewMatrix()));
	glUniformMatrix4fv(glGetUniformLocation(id, "projection"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.projection));
	glUniform1i(glGetUniformLocation(id, "theTexture"), 0);

	// normalMatrix (computed above) corrects normals under non-uniform scale.
	glUniformMatrix3fv(glGetUniformLocation(id, "normalMatrix"), 1, GL_FALSE,
		glm::value_ptr(normalMatrix));
	glUniform3fv(glGetUniformLocation(id, "viewPos"), 1,
		glm::value_ptr(renderer.camera.position));
	glUniform1i(glGetUniformLocation(id, "toon"), scene.celShading ? 1 : 0);
	// Seconds since start, for animated materials (water ripples).
	glUniform1f(glGetUniformLocation(id, "uTime"), renderer.now * 0.001f);

	// Material uniforms (mat resolved above; default matte material as fallback).
	glUniform3fv(glGetUniformLocation(id, "matTint"), 1, glm::value_ptr(mat.tint));
	glUniform3fv(glGetUniformLocation(id, "matEmissive"), 1, glm::value_ptr(mat.emissive));
	glUniform1f(glGetUniformLocation(id, "matFresnel"), mat.fresnel);
	glUniform2fv(glGetUniformLocation(id, "matUVTile"), 1, glm::value_ptr(mat.uvTile));
	glUniform1f(glGetUniformLocation(id, "matNormalStrength"), mat.normalStrength);
	glUniform1i(glGetUniformLocation(id, "matNormalMode"), (int)mat.normalMode);
	glUniform1i(glGetUniformLocation(id, "matLighting"), (int)mat.lighting);
	glUniform1f(glGetUniformLocation(id, "matSpecular"), mat.specular);
	glUniform1f(glGetUniformLocation(id, "matShininess"), mat.shininess);
	glUniform1f(glGetUniformLocation(id, "matMetallic"), mat.metallic);
	glUniform1f(glGetUniformLocation(id, "matRoughness"), mat.roughness);
	glUniform1f(glGetUniformLocation(id, "matOpacity"), mat.opacity);

	// Per-surface water tuning (drives the vertex waves + fragment water look).
	// Overrides the material's specular/shininess/opacity so lakes sharing one
	// water material can still be tuned individually in the editor.
	if (mat.lighting == LightingModel::Water)
	{
		glUniform1f(glGetUniformLocation(id, "uWaterAmp"), water.amplitude);
		glUniform1f(glGetUniformLocation(id, "uWaterWaveScale"), water.waveScale);
		glUniform1f(glGetUniformLocation(id, "uWaterShoreFade"), water.shoreFade);
		glUniform1f(glGetUniformLocation(id, "uWaterChoppy"), water.choppy);
		glUniform1f(glGetUniformLocation(id, "matSpecular"), water.specular);
		glUniform1f(glGetUniformLocation(id, "matShininess"), water.shininess);
		glUniform1f(glGetUniformLocation(id, "matOpacity"), water.opacity);
	}

	// Bind the normal map to unit 1 (if any), then restore unit 0 so the rest
	// of the pipeline's single-unit texture caching stays consistent.
	if (mat.normalMap != nullptr)
	{
		glUniform1i(glGetUniformLocation(id, "matHasNormal"), 1);
		glUniform1i(glGetUniformLocation(id, "normalMap"), 1);
		mat.normalMap->UseTexture(GL_TEXTURE1);
		glActiveTexture(GL_TEXTURE0);
	}
	else
	{
		glUniform1i(glGetUniformLocation(id, "matHasNormal"), 0);
	}

	Scene3D::Get().ApplyLighting(id);

	texture->UseTexture();

	for (Mesh* mesh : model3D.meshList)
	{
		mesh->RenderMesh(0);
	}

	renderer.drawCallsPerFrame++;
#endif
}

// Transparent pass: draw every model whose material has opacity < 1, sorted
// back-to-front, with depth writes off (so overlapping glass/ice blends in the
// right order without occluding itself). Called by Game after the opaque 3D
// pass, while depth testing is still on. No-op with no transparent models.
void Scene3D::RenderTransparentModels(Game& game, const Renderer& renderer)
{
	if (!active || renderer.camera.useOrthoCamera)
		return;

	std::vector<Scene3DModel*> transparent;
	for (Scene3DModel* m : models)
		if (m->material != nullptr && m->material->IsTransparent())
			transparent.push_back(m);
	if (transparent.empty())
		return;

	// Sort back-to-front by squared distance from the camera.
	glm::vec3 camPos = renderer.camera.position;
	std::sort(transparent.begin(), transparent.end(),
		[&](Scene3DModel* a, Scene3DModel* b)
		{
			float da = glm::dot(a->position - camPos, a->position - camPos);
			float db = glm::dot(b->position - camPos, b->position - camPos);
			return da > db;   // farthest first
		});

	glDepthMask(GL_FALSE);   // don't write depth; keep depth TEST on
	for (Scene3DModel* m : transparent)
		m->DrawGeometry(renderer);
	glDepthMask(GL_TRUE);
}

void Scene3D::EnsureShadowMap()
{
	if (shadowFBO != 0)
		return;
	glGenFramebuffers(1, &shadowFBO);
	glGenTextures(1, &shadowDepthTex);
	glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowMapSize, shadowMapSize,
		0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f };   // outside frustum = far = lit
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
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

	// Orthographic light frustum covering the scene (centred on the origin, a
	// little below ground so it spans the props' height; up is -Y).
	const float R = 3200.0f;                 // half-size of the covered area
	glm::vec3 center(0.0f, -150.0f, 0.0f);
	glm::vec3 eye = center - L * (R * 1.6f);
	glm::vec3 up = (std::fabs(L.y) > 0.97f) ? glm::vec3(0, 0, 1) : glm::vec3(0, -1, 0);
	glm::mat4 lightView = glm::lookAt(eye, center, up);
	glm::mat4 lightProj = glm::ortho(-R, R, -R, R, 1.0f, R * 3.5f);
	lightSpaceMatrix = lightProj * lightView;

	glViewport(0, 0, shadowMapSize, shadowMapSize);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	// Depth writes only happen with the depth test ENABLED; at the start of a
	// frame it may be off (left by the 2D/GUI pass), which would leave the shadow
	// map empty (no shadows at all). Force just the depth state here - do NOT
	// touch GL_BLEND (the frame's final compositing needs it; disabling it here
	// blacked out the screen).
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	shadowDepthShader->UseShader();
	GLuint id = shadowDepthShader->GetID();
	glUniformMatrix4fv(glGetUniformLocation(id, "lightSpace"), 1, GL_FALSE,
		glm::value_ptr(lightSpaceMatrix));
	glUniform1i(glGetUniformLocation(id, "theTexture"), 0);
	glUniform1f(glGetUniformLocation(id, "alphaCutoff"), 0.5f);

	// Casters: models with real vertical extent (skips flat ground/sidewalks/
	// lakebed) and not the water surface.
	for (Scene3DModel* m : models)
	{
		if (m == nullptr || !m->loaded || m->texture == nullptr || m->IsWater())
			continue;
		if (std::fabs(m->aabbMax.y - m->aabbMin.y) < 15.0f)
			continue;
		glm::mat4 model(1.0f);
		model = glm::translate(model, m->position);
		model = glm::rotate(model, glm::radians(m->yawDeg), glm::vec3(0, -1, 0));
		model = glm::rotate(model, glm::radians(m->pitchDeg), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(m->rollDeg), glm::vec3(0, 0, 1));
		model = glm::scale(model, m->EffectiveScale());
		glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(model));
		m->texture->UseTexture();
		for (Mesh* mesh : m->model3D.meshList)
			mesh->RenderMesh(0);
	}

	// Character casters: an upright sprite quad whose width runs across the sun,
	// so the light sees the full silhouette (person-shaped shadow).
	glm::vec3 worldUp(0.0f, -1.0f, 0.0f);
	glm::vec3 sunH(L.x, 0.0f, L.z);
	if (glm::length(sunH) < 1e-4f) sunH = glm::vec3(0, 0, 1);
	sunH = glm::normalize(sunH);
	glm::vec3 right = glm::normalize(glm::cross(sunH, worldUp));
	for (Character3D* ch : characters)
	{
		if (ch == nullptr || ch->quad == nullptr || ch->bodyTex == nullptr)
			continue;
		float aspect = (ch->bodyTex->GetHeight() > 0)
			? (float)ch->bodyTex->GetWidth() / (float)ch->bodyTex->GetHeight() : 0.5f;
		float width = ch->worldHeight * aspect;
		glm::mat4 model(1.0f);
		model[0] = glm::vec4(right * width, 0.0f);
		model[1] = glm::vec4(worldUp * ch->worldHeight, 0.0f);
		model[2] = glm::vec4(sunH, 0.0f);
		model[3] = glm::vec4(ch->position, 1.0f);
		glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(model));
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
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, game.screenWidth, game.screenHeight);
	shadowActive = true;
#endif
}

std::vector<std::string> Scene3D::PointLightNames() const
{
	std::vector<std::string> names;
	for (const ScenePointLight& p : pointLights)
		names.push_back(p.name);
	return names;
}

void Scene3D::EnsurePointShadowMaps()
{
	if (pointShadowFBO == 0)
		glGenFramebuffers(1, &pointShadowFBO);
	for (int c = 0; c < kMaxPointShadows; c++)
	{
		if (pointShadowCubes[c] != 0) continue;
		glGenTextures(1, &pointShadowCubes[c]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCubes[c]);
		for (int i = 0; i < 6; i++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
				pointShadowSize, pointShadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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

	// Choose the casters: a specific named light (only that one), else AUTO = all
	// enabled point lights, strongest first, up to kMaxPointShadows.
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
			if (pointLights[i].on) on.push_back((int)i);
		std::sort(on.begin(), on.end(), [&](int a, int b) {
			return pointLights[a].intensity * pointLights[a].range
			     > pointLights[b].intensity * pointLights[b].range;
		});
		for (int idx : on)
		{
			if ((int)casters.size() >= kMaxPointShadows) break;
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
		if (m && m->loaded && !m->IsWater())
			sig += m->position.x * 1.7 + m->position.y * 2.9 + m->position.z * 3.1;
	for (Character3D* ch : characters)
		if (ch)
			sig += ch->position.x * 1.3 + ch->position.y * 2.1 + ch->position.z * 4.3;

	if (pointShadowEverRendered && sig == pointShadowSig)
	{
		pointShadowActive = true;   // reuse the cached cubes; no depth work this frame
		return;
	}
	pointShadowSig = sig;
	pointShadowEverRendered = true;

	pointShadowShader->UseShader();
	GLuint id = pointShadowShader->GetID();
	glUniform1i(glGetUniformLocation(id, "theTexture"), 0);
	glUniform1f(glGetUniformLocation(id, "alphaCutoff"), 0.5f);

	glViewport(0, 0, pointShadowSize, pointShadowSize);
	glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

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
		glUniform3fv(glGetUniformLocation(id, "lightPos"), 1, glm::value_ptr(P));
		glUniform1f(glGetUniformLocation(id, "farPlane"), farP);

		for (int face = 0; face < 6; face++)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, pointShadowCubes[s], 0);
			glClear(GL_DEPTH_BUFFER_BIT);
			glm::mat4 vp = proj * views[face];
			glUniformMatrix4fv(glGetUniformLocation(id, "viewProj"), 1, GL_FALSE, glm::value_ptr(vp));

			// Models with real height, RANGE-CULLED: skip anything whose bounding
			// sphere is entirely beyond this light's reach (its shadow can't land
			// on a lit surface).
			for (Scene3DModel* m : models)
			{
				if (m == nullptr || !m->loaded || m->texture == nullptr || m->IsWater())
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
				glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(model));
				m->texture->UseTexture();
				for (Mesh* mesh : m->model3D.meshList)
					mesh->RenderMesh(0);
			}

			// Characters: upright sprite quad facing the light (alpha silhouette),
			// range-culled by the light's reach.
			for (Character3D* ch : characters)
			{
				if (ch == nullptr || ch->quad == nullptr || ch->bodyTex == nullptr)
					continue;
				if (glm::length(ch->position - P) - ch->worldHeight * 0.5f > farP)
					continue;
				glm::vec3 toLight = P - ch->position; toLight.y = 0.0f;
				if (glm::length(toLight) < 1e-4f) toLight = glm::vec3(0, 0, 1);
				toLight = glm::normalize(toLight);
				glm::vec3 right = glm::normalize(glm::cross(toLight, worldUp));
				float aspect = (ch->bodyTex->GetHeight() > 0)
					? (float)ch->bodyTex->GetWidth() / (float)ch->bodyTex->GetHeight() : 0.5f;
				float width = ch->worldHeight * aspect;
				glm::mat4 model(1.0f);
				model[0] = glm::vec4(right * width, 0.0f);
				model[1] = glm::vec4(worldUp * ch->worldHeight, 0.0f);
				model[2] = glm::vec4(toLight, 0.0f);
				model[3] = glm::vec4(ch->position, 1.0f);
				glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(model));
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

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, game.screenWidth, game.screenHeight);
	pointShadowActive = true;
#endif
}

// ------------------------------------------------------------ characters

Character3D::Character3D(const glm::vec3& pos) : Entity(pos)
{
	etype = "character3d";
	name = "character3d";
	layer = DrawingLayer::MIDDLE;   // in front of scene models, behind the GUI
	drawOrder = 0;
}

void Character3D::Update(Game& game)
{
	Entity::Update(game);
}

void Character3D::DrawQuad(const Renderer& renderer, Texture* tex, float forwardBias)
{
	if (tex == nullptr || quad == nullptr || shader == nullptr)
		return;

	// Upright billboard model matrix. The shared unit quad spans local
	// x[-0.5,0.5], y[0,1] (base at 0, growing to the top), z=0. We map:
	//   local x -> camera-facing "right" scaled to the sprite width
	//   local y -> visual up (world -Y) scaled to worldHeight
	//   local z -> direction to the camera (used only for the head bias)
	// so the sprite stays vertical and only yaws to face the camera.
	glm::vec3 worldUp(0.0f, -1.0f, 0.0f);
	glm::vec3 toCam = renderer.camera.position - position;
	toCam.y = 0.0f;  // yaw-only billboard: ignore camera height
	if (glm::length(toCam) < 0.0001f)
		toCam = glm::vec3(0, 0, 1);
	toCam = glm::normalize(toCam);

	// cross(toCam, worldUp) (not worldUp x toCam) so local +x maps to screen
	// right and the texture isn't horizontally mirrored
	glm::vec3 right = glm::normalize(glm::cross(toCam, worldUp));

	float aspect = (tex->GetHeight() > 0)
		? (float)tex->GetWidth() / (float)tex->GetHeight() : 0.5f;
	float width = worldHeight * aspect;

	glm::mat4 m(1.0f);
	m[0] = glm::vec4(right * width, 0.0f);
	m[1] = glm::vec4(worldUp * worldHeight, 0.0f);
	m[2] = glm::vec4(toCam, 0.0f);
	// Base anchored on the floor at position; head layer nudged toward the
	// camera by forwardBias so it wins the depth test against the body
	m[3] = glm::vec4(position + toCam * forwardBias, 1.0f);

	shader->UseShader();
	GLuint id = shader->GetID();
	glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, glm::value_ptr(m));
	glUniformMatrix4fv(glGetUniformLocation(id, "view"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.CalculateViewMatrix()));
	glUniformMatrix4fv(glGetUniformLocation(id, "projection"), 1, GL_FALSE,
		glm::value_ptr(renderer.camera.projection));
	glUniform1i(glGetUniformLocation(id, "theTexture"), 0);

	Scene3D::Get().ApplyLighting(id);

	tex->UseTexture();
	quad->RenderMesh(0);
	renderer.drawCallsPerFrame++;
}

void Character3D::Render(const Renderer& renderer)
{
	if (renderer.camera.useOrthoCamera)
		return;

	// Body (or combined sprite) first, then head layered on top. Both are
	// full-canvas overlays that align by transparency; the head is pulled
	// slightly toward the camera so it isn't z-culled by the coplanar body.
	DrawQuad(renderer, bodyTex, 0.0f);
	if (headTex != nullptr)
		DrawQuad(renderer, headTex, 1.5f);
}

// --------------------------------------------------------------- manager

Scene3D& Scene3D::Get()
{
	static Scene3D instance;
	return instance;
}

bool Scene3D::Load(Game& game, const std::string& sceneName)
{
	std::string path = "data/scenes/" + sceneName + ".scene";
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cout << "Scene3D: cannot open " << path << std::endl;
		return false;
	}
	return LoadFromStream(game, file, sceneName, true);
}

bool Scene3D::LoadFromString(Game& game, const std::string& text, const std::string& sceneName, bool jumpCamera)
{
	std::istringstream ss(text);
	return LoadFromStream(game, ss, sceneName, jumpCamera);
}

bool Scene3D::LoadFromStream(Game& game, std::istream& file, const std::string& sceneName, bool jumpCamera)
{
	// Replace any scene already showing (keeps the camera in 3D mode)
	if (active)
	{
		for (Scene3DModel* m : models)
		{
			game.ShouldDeleteEntity(m);
		}
		for (Character3D* ch : characters)
		{
			game.ShouldDeleteEntity(ch);
		}
		if (skybox != nullptr)
		{
			game.ShouldDeleteEntity(skybox);
			skybox = nullptr;
		}
		models.clear();
		characters.clear();
		solids.clear();
		cameras.clear();
		cameraOrder.clear();
	}

	// Dedicated shaders (all the VN shaders are 2D): lit for scene models,
	// unlit alpha-cutout for character billboards
	if (shader == nullptr)
	{
		shader = new ShaderProgram(-1, "data/shaders/scene3d.vert", "data/shaders/scene3d.frag");
	}
	if (billboardShader == nullptr)
	{
		billboardShader = new ShaderProgram(-1, "data/shaders/billboard3d.vert", "data/shaders/billboard3d.frag");
	}
	if (edgeShader == nullptr)
	{
		edgeShader = new ShaderProgram(-1, "data/shaders/scene3d_edge.vert", "data/shaders/scene3d_edge.frag");
	}
	if (shadowDepthShader == nullptr)
	{
		shadowDepthShader = new ShaderProgram(-1, "data/shaders/shadow_depth.vert", "data/shaders/shadow_depth.frag");
	}
	if (pointShadowShader == nullptr)
	{
		pointShadowShader = new ShaderProgram(-1, "data/shaders/point_shadow_depth.vert", "data/shaders/point_shadow_depth.frag");
	}

	// (Re)load the material library so "mat <name>" tokens can resolve.
	MaterialLibrary::Get().Load(game);

	// Reset lighting to defaults; the file's light directives override it
	ambientColor = glm::vec3(0.08f, 0.08f, 0.10f);
	dirLight = SceneDirLight();
	pointLights.clear();
	spotLights.clear();
	shadowCasterLight.clear();   // reset the caster override per scene
	pointShadowEverRendered = false;   // force the point-shadow cubes to re-render
	skyTexPath.clear();   // a scene without a "sky" line has none

	// Shared unit billboard quad: x[-0.5,0.5], y[0,1] (base at origin), z=0,
	// with dummy normals so Mesh::CreateMesh's stride-8 layout is satisfied
	if (billboardQuad == nullptr)
	{
		GLfloat qv[] = {
			// pos                uv          normal
			-0.5f, 0.0f, 0.0f,   0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
			 0.5f, 0.0f, 0.0f,   1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
			 0.5f, 1.0f, 0.0f,   1.0f, 0.0f,  0.0f, 0.0f, 1.0f,
			-0.5f, 1.0f, 0.0f,   0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
		};
		GLuint qi[] = { 0, 1, 2, 0, 2, 3 };
		billboardQuad = new Mesh();
		billboardQuad->CreateMesh(qv, qi, 32, 6, 8, 3, 5);
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream ss(line);
		std::string tag;
		ss >> tag;

		if (tag == "model")
		{
			// model <obj> <tex> <x> <y> <z> <yaw> <scale> [solid] [tag <VALUE>]
			// The trailing "solid" flag and "tag <VALUE>" are optional and
			// order-independent.
			std::string objPath, texPath;
			glm::vec3 pos;
			float yaw = 0.0f, scale = 1.0f;
			ss >> objPath >> texPath >> pos.x >> pos.y >> pos.z >> yaw >> scale;

			Scene3DModel* m = new Scene3DModel(pos);
			m->yawDeg = yaw;
			m->modelScale = scale;
			m->shader = shader;
			m->texture = game.spriteManager.GetImage(texPath, Texture::Filter::Smooth);
			m->objPath = objPath;
			m->texPath = texPath;

			std::string flag;
			while (ss >> flag)
			{
				if (flag == "solid")
					m->solid = true;
				else if (flag == "tag" || flag == "clue")   // "clue" = back-compat
					ss >> m->interactionTag;
				else if (flag == "rot")            // rot <pitch> <roll> (extra axes)
					ss >> m->pitchDeg >> m->rollDeg;
				else if (flag == "scaleaxis")      // scaleaxis <sx> <sy> <sz>
					ss >> m->scaleAxis.x >> m->scaleAxis.y >> m->scaleAxis.z;
				else if (flag == "mat")            // mat <material name>
					ss >> m->materialName;
				else if (flag == "water")          // water <amp> <scale> <shore> <choppy> <spec> <shin> <opacity>
					ss >> m->water.amplitude >> m->water.waveScale >> m->water.shoreFade
					   >> m->water.choppy >> m->water.specular >> m->water.shininess >> m->water.opacity;
			}
			if (!m->materialName.empty())
				m->material = MaterialLibrary::Get().Find(m->materialName);
#ifdef USE_ASSIMP
			m->model3D.LoadModel(objPath);
			m->loaded = !m->model3D.meshList.empty();
#endif
			if (!m->loaded || m->texture == nullptr)
			{
				std::cout << "Scene3D: failed to load model " << objPath
					<< " (tex " << texPath << ")" << std::endl;
			}

			// Cache OBJ-space bounds and compute the initial world pick AABB.
			m->hasLocalBounds = ReadObjLocalAABB(objPath, m->localMin, m->localMax);
			RecomputeModelBounds(m);

			models.push_back(m);
			game.entities.push_back(m);

			// Solid furniture: build a world XZ footprint characters avoid
			if (m->solid)
			{
				SolidBox box;
				if (ComputeSolidBox(objPath, pos, yaw, scale, box))
				{
					solids.push_back(box);
					std::cout << "Scene3D: solid " << objPath << " footprint x["
						<< box.minX << "," << box.maxX << "] z["
						<< box.minZ << "," << box.maxZ << "]" << std::endl;
				}
			}
		}
		else if (tag == "camera")
		{
			std::string camName;
			CamPose pose;
			ss >> camName >> pose.position.x >> pose.position.y >> pose.position.z
				>> pose.pitch >> pose.yaw;
			cameras[camName] = pose;
			cameraOrder.push_back(camName);
		}
		else if (tag == "ambient")
		{
			// ambient <r> <g> <b>  - global fill (keep low for a dark room)
			ss >> ambientColor.r >> ambientColor.g >> ambientColor.b;
		}
		else if (tag == "sky")
		{
			// sky <equirectangular texture> [radius]  - a camera-following
			// panorama sphere behind everything (e.g. a sunset backdrop).
			std::string skyTex;
			float radius = 4000.0f;
			ss >> skyTex;
			if (ss >> radius) {}   // optional
			skyTexPath = skyTex;
			skyRadiusVal = radius;
			skybox = new Skybox(game, skyTex, radius);
			game.entities.push_back(skybox);
		}
		else if (tag == "light")
		{
			// light <dx> <dy> <dz> <colR> <colG> <colB> <diffuse>
			// Optional directional fill; diffuse defaults 0 = off. Dir points
			// from the light into the scene (visual up = -Y, so a downward
			// key light has +dy).
			SceneDirLight L;
			ss >> L.dir.x >> L.dir.y >> L.dir.z
				>> L.color.r >> L.color.g >> L.color.b >> L.diffuse;
			dirLight = L;
		}
		else if (tag == "point")
		{
			// point <name> <x> <y> <z> <r> <g> <b> <range> <intensity>
			ScenePointLight p;
			ss >> p.name >> p.pos.x >> p.pos.y >> p.pos.z
				>> p.color.r >> p.color.g >> p.color.b
				>> p.range >> p.intensity;
			pointLights.push_back(p);
		}
		else if (tag == "spot")
		{
			// spot <name> <x> <y> <z> <dx> <dy> <dz> <r> <g> <b>
			//      <range> <intensity> <innerDeg> <outerDeg>
			SceneSpotLight s;
			ss >> s.name >> s.pos.x >> s.pos.y >> s.pos.z
				>> s.dir.x >> s.dir.y >> s.dir.z
				>> s.color.r >> s.color.g >> s.color.b
				>> s.range >> s.intensity >> s.innerDeg >> s.outerDeg;
			spotLights.push_back(s);
		}
		else if (tag == "shadowlight")   // shadowlight <point-light name> (caster override)
		{
			ss >> shadowCasterLight;
		}
		else if (tag == "character")
		{
			// Two forms (name comes first so "scene3d focus <name>" works):
			//   character <name> layered  <folder> <bodyPose> <headExpr> <x> <y> <z> <height>
			//   character <name> combined <spritePath>                    <x> <y> <z> <height>
			std::string charName, mode;
			ss >> charName >> mode;

			Character3D* ch = nullptr;

			std::string bodyPath, headPath;

			if (mode == "layered")
			{
				std::string folder, bodyPose, headExpr;
				glm::vec3 pos;
				float height = 220.0f;
				ss >> folder >> bodyPose >> headExpr
					>> pos.x >> pos.y >> pos.z >> height;

				ch = new Character3D(pos);
				ch->worldHeight = height;
				ch->bodyTex = ResolveTexture(game, folder, "body", bodyPose, &bodyPath);
				ch->headTex = ResolveTexture(game, folder, "head", headExpr, &headPath);
				ch->mode = "layered";
				ch->folder = folder;
				ch->bodyPose = bodyPose;
				ch->headExpr = headExpr;
			}
			else if (mode == "combined")
			{
				std::string spritePath;
				glm::vec3 pos;
				float height = 220.0f;
				ss >> spritePath >> pos.x >> pos.y >> pos.z >> height;

				ch = new Character3D(pos);
				ch->worldHeight = height;
				ch->bodyTex = game.spriteManager.GetImage(spritePath, Texture::Filter::Smooth);
				ch->headTex = nullptr;  // combined sprites need no layering
				bodyPath = spritePath;
				ch->mode = "combined";
				ch->spritePath = spritePath;
			}

			if (ch != nullptr)
			{
				ch->charName = charName;
				ComputeFigureBounds(ch, bodyPath, headPath);
				ch->shader = billboardShader;
				ch->quad = billboardQuad;
				if (ch->bodyTex == nullptr)
					std::cout << "Scene3D: character has no body/sprite texture" << std::endl;
				characters.push_back(ch);
				game.entities.push_back(ch);
			}
		}
	}

	// Now that every solid is known (order-independent), push each character
	// clear of solid furniture so no one is left clipping into it
	for (Character3D* ch : characters)
	{
		glm::vec3 before = ch->position;
		ResolveAgainstSolids(ch->position, ch->collisionRadius);
		if (ch->position != before)
		{
			std::cout << "Scene3D: pushed a character out of a solid, ("
				<< before.x << "," << before.z << ") -> ("
				<< ch->position.x << "," << ch->position.z << ")" << std::endl;
		}
	}

	std::cout << "Scene3D: loaded " << sceneName << " (" << models.size()
		<< " models, " << characters.size() << " characters, "
		<< cameras.size() << " cameras)" << std::endl;
	std::cout << "Scene3D: lighting - ambient (" << ambientColor.r << ","
		<< ambientColor.g << "," << ambientColor.b << "), "
		<< pointLights.size() << " point, " << spotLights.size()
		<< " spot, dir diffuse " << dirLight.diffuse << std::endl;

	if (!active)
	{
		EnterPerspective(game);
	}
	active = true;
	sceneEverLoaded = true;
	currentScene = sceneName;
	gliding = false;

	if (jumpCamera && !cameraOrder.empty())
	{
		JumpToCamera(game, cameraOrder[0]);
	}
	return true;
}


// Resolve a body/head sprite. Handles both current layouts:
//   assets/sprites/<folder>/<part>/<code>.png       (e.g. Maria head/dd.png)
//   assets/sprites/<folder>/<part>/<code>/1.png      (e.g. Butler head/hh/1.png)
Texture* Scene3D::ResolveTexture(Game& game, const std::string& folder,
	const std::string& part, const std::string& code, std::string* resolvedPath)
{
	std::string base = "assets/sprites/" + folder + "/" + part + "/" + code;

	std::string flat = base + ".png";
	std::ifstream probe(flat);
	if (probe.good())
	{
		probe.close();
		if (resolvedPath) *resolvedPath = flat;
		return game.spriteManager.GetImage(flat, Texture::Filter::Smooth);
	}

	// Animation-frame folder: use the first frame as the static pose
	std::string framed = base + "/1.png";
	std::ifstream probe2(framed);
	if (probe2.good())
	{
		probe2.close();
		if (resolvedPath) *resolvedPath = framed;
		return game.spriteManager.GetImage(framed, Texture::Filter::Smooth);
	}

	std::cout << "Scene3D: sprite not found: " << flat << " (or " << framed << ")" << std::endl;
	return nullptr;
}

// Opaque bounds of an image, as fractions of the canvas: vertical from the
// TOP (0 = top edge, 1 = bottom), horizontal from the LEFT (0 = left edge,
// 1 = right). Returns false if unreadable or fully transparent.
static bool AlphaBounds(const std::string& path, float& topFromTop, float& botFromTop,
	float& leftFromLeft, float& rightFromLeft)
{
	SDL_Surface* raw = IMG_Load(path.c_str());
	if (raw == nullptr)
		return false;
	SDL_Surface* s = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(raw);
	if (s == nullptr)
		return false;

	const int W = s->w, H = s->h, pitch = s->pitch;
	const Uint8* px = static_cast<const Uint8*>(s->pixels);
	int minY = H, maxY = -1, minX = W, maxX = -1;
	for (int y = 0; y < H; y++)
	{
		const Uint8* row = px + (size_t)y * pitch;
		for (int x = 0; x < W; x++)
		{
			if (row[x * 4 + 3] > 16)  // alpha threshold
			{
				if (y < minY) minY = y;
				if (y > maxY) maxY = y;
				if (x < minX) minX = x;
				if (x > maxX) maxX = x;
			}
		}
	}
	SDL_FreeSurface(s);
	if (maxY < 0 || H <= 0 || W <= 0)
		return false;

	topFromTop = (float)minY / (float)H;
	botFromTop = (float)(maxY + 1) / (float)H;
	leftFromLeft = (float)minX / (float)W;
	rightFromLeft = (float)(maxX + 1) / (float)W;
	return true;
}

void Scene3D::ComputeFigureBounds(Character3D* ch,
	const std::string& bodyPath, const std::string& headPath) const
{
	// Union the opaque bounds of every sprite that makes up the figure.
	// Vertical fractions from the canvas top, horizontal from the left.
	float topFromTop = 1.0f, botFromTop = 0.0f, leftFromLeft = 1.0f, rightFromLeft = 0.0f;
	bool any = false;

	float t, b, l, r;
	if (!bodyPath.empty() && AlphaBounds(bodyPath, t, b, l, r))
	{
		topFromTop = any ? std::min(topFromTop, t) : t;
		botFromTop = any ? std::max(botFromTop, b) : b;
		leftFromLeft = any ? std::min(leftFromLeft, l) : l;
		rightFromLeft = any ? std::max(rightFromLeft, r) : r;
		any = true;
	}
	if (!headPath.empty() && AlphaBounds(headPath, t, b, l, r))
	{
		topFromTop = any ? std::min(topFromTop, t) : t;
		botFromTop = any ? std::max(botFromTop, b) : b;
		leftFromLeft = any ? std::min(leftFromLeft, l) : l;
		rightFromLeft = any ? std::max(rightFromLeft, r) : r;
		any = true;
	}

	if (!any)
		return;  // keep defaults

	// Canvas top -> billboard-from-base (feet at billboard 0, head near 1):
	// a small "from top" (near the head) maps to a large fraction from base.
	ch->figureTopFrac = 1.0f - topFromTop;  // head
	ch->figureBotFrac = 1.0f - botFromTop;  // feet
	// Horizontal center of the opaque region as a fraction from the canvas
	// left (used to center the DRAWN body, not the placement point).
	ch->figureHCenterFrac = 0.5f * (leftFromLeft + rightFromLeft);
	std::cout << "Scene3D: '" << ch->charName << "' figure bounds top "
		<< ch->figureTopFrac << " bot " << ch->figureBotFrac
		<< " hcenter " << ch->figureHCenterFrac << std::endl;
}

void Scene3D::ApplyLighting(unsigned int shaderID) const
{
	const int MAX_POINTS = 8;
	const int MAX_SPOTS = 4;
	GLuint id = (GLuint)shaderID;

	glUniform3fv(glGetUniformLocation(id, "ambientColor"), 1, glm::value_ptr(ambientColor));
	glUniform3fv(glGetUniformLocation(id, "dirLightDir"), 1, glm::value_ptr(dirLight.dir));
	glUniform3fv(glGetUniformLocation(id, "dirLightColor"), 1, glm::value_ptr(dirLight.color));
	glUniform1f(glGetUniformLocation(id, "dirLightDiffuse"), dirLight.diffuse);

	// Sun shadow map (bound to unit 3; unit 0 = albedo, 1 = normal map).
	if (shadowActive && shadowsEnabled && shadowDepthTex != 0)
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(id, "shadowMap"), 3);
		glUniformMatrix4fv(glGetUniformLocation(id, "lightSpaceMatrix"), 1, GL_FALSE,
			glm::value_ptr(lightSpaceMatrix));
		glUniform1f(glGetUniformLocation(id, "shadowStrength"), shadowStrength);
		glUniform1i(glGetUniformLocation(id, "shadowsOn"), 1);
	}
	else
	{
		glUniform1i(glGetUniformLocation(id, "shadowsOn"), 0);
	}

	// Point lights: pack the ENABLED ones into contiguous arrays (off
	// lights are skipped, shrinking the uploaded count)
	float pPos[MAX_POINTS * 3], pCol[MAX_POINTS * 3], pRange[MAX_POINTS], pInt[MAX_POINTS];
	int pc = 0;
	// Map each cube-shadow caster slot -> its packed point-light index.
	int packedForSlot[kMaxPointShadows];
	for (int s = 0; s < kMaxPointShadows; s++) packedForSlot[s] = -1;
	for (const ScenePointLight& p : pointLights)
	{
		if (!p.on || pc >= MAX_POINTS) continue;
		pPos[pc * 3 + 0] = p.pos.x; pPos[pc * 3 + 1] = p.pos.y; pPos[pc * 3 + 2] = p.pos.z;
		pCol[pc * 3 + 0] = p.color.r; pCol[pc * 3 + 1] = p.color.g; pCol[pc * 3 + 2] = p.color.b;
		pRange[pc] = p.range; pInt[pc] = p.intensity;
		if (pointShadowActive)
			for (int s = 0; s < pointShadowCount; s++)
				if (p.pos == pointShadowPositions[s]) packedForSlot[s] = pc;
		pc++;
	}
	glUniform1i(glGetUniformLocation(id, "pointCount"), pc);
	if (pc > 0)
	{
		glUniform3fv(glGetUniformLocation(id, "pointPos"), pc, pPos);
		glUniform3fv(glGetUniformLocation(id, "pointColor"), pc, pCol);
		glUniform1fv(glGetUniformLocation(id, "pointRange"), pc, pRange);
		glUniform1fv(glGetUniformLocation(id, "pointIntensity"), pc, pInt);
	}

	// Point-light (cube) shadows: bind each caster's cube to units 4,5,6,... and
	// upload the parallel arrays (position, far, and the packed light index it
	// shadows). shadowStrength is shared with the sun shadow.
	if (pointShadowActive && pointShadowsEnabled && pointShadowCount > 0)
	{
		float psPos[kMaxPointShadows * 3], psFar[kMaxPointShadows];
		int psIdx[kMaxPointShadows], psUnit[kMaxPointShadows];
		int n = 0;
		for (int s = 0; s < pointShadowCount; s++)
		{
			if (pointShadowCubes[s] == 0 || packedForSlot[s] < 0) continue;
			glActiveTexture(GL_TEXTURE4 + n);
			glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCubes[s]);
			psUnit[n] = 4 + n;
			psPos[n * 3 + 0] = pointShadowPositions[s].x;
			psPos[n * 3 + 1] = pointShadowPositions[s].y;
			psPos[n * 3 + 2] = pointShadowPositions[s].z;
			psFar[n] = pointShadowFars[s];
			psIdx[n] = packedForSlot[s];
			n++;
		}
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(id, "pointShadowCount"), n);
		if (n > 0)
		{
			glUniform1iv(glGetUniformLocation(id, "pointShadowMaps"), n, psUnit);
			glUniform3fv(glGetUniformLocation(id, "pointShadowPositions"), n, psPos);
			glUniform1fv(glGetUniformLocation(id, "pointShadowFars"), n, psFar);
			glUniform1iv(glGetUniformLocation(id, "pointShadowLightIdx"), n, psIdx);
			glUniform1f(glGetUniformLocation(id, "shadowStrength"), shadowStrength);
		}
	}
	else
	{
		glUniform1i(glGetUniformLocation(id, "pointShadowCount"), 0);
	}

	// Spot lights (enabled only)
	float sPos[MAX_SPOTS * 3], sDir[MAX_SPOTS * 3], sCol[MAX_SPOTS * 3];
	float sRange[MAX_SPOTS], sInt[MAX_SPOTS], sCosIn[MAX_SPOTS], sCosOut[MAX_SPOTS];
	int sc = 0;
	for (const SceneSpotLight& s : spotLights)
	{
		if (!s.on || sc >= MAX_SPOTS) continue;
		sPos[sc * 3 + 0] = s.pos.x; sPos[sc * 3 + 1] = s.pos.y; sPos[sc * 3 + 2] = s.pos.z;
		glm::vec3 d = glm::normalize(s.dir);
		sDir[sc * 3 + 0] = d.x; sDir[sc * 3 + 1] = d.y; sDir[sc * 3 + 2] = d.z;
		sCol[sc * 3 + 0] = s.color.r; sCol[sc * 3 + 1] = s.color.g; sCol[sc * 3 + 2] = s.color.b;
		sRange[sc] = s.range; sInt[sc] = s.intensity;
		sCosIn[sc] = cosf(glm::radians(s.innerDeg));
		sCosOut[sc] = cosf(glm::radians(s.outerDeg));
		sc++;
	}
	glUniform1i(glGetUniformLocation(id, "spotCount"), sc);
	if (sc > 0)
	{
		glUniform3fv(glGetUniformLocation(id, "spotPos"), sc, sPos);
		glUniform3fv(glGetUniformLocation(id, "spotDir"), sc, sDir);
		glUniform3fv(glGetUniformLocation(id, "spotColor"), sc, sCol);
		glUniform1fv(glGetUniformLocation(id, "spotRange"), sc, sRange);
		glUniform1fv(glGetUniformLocation(id, "spotIntensity"), sc, sInt);
		glUniform1fv(glGetUniformLocation(id, "spotCosInner"), sc, sCosIn);
		glUniform1fv(glGetUniformLocation(id, "spotCosOuter"), sc, sCosOut);
	}
}

// --- solid collision ------------------------------------------------------

bool Scene3D::ReadObjLocalAABB(const std::string& objPath,
	glm::vec3& lo, glm::vec3& hi) const
{
	std::ifstream file(objPath);
	if (!file.is_open())
	{
		std::cout << "Scene3D: cannot open OBJ " << objPath << std::endl;
		return false;
	}

	lo = glm::vec3(1e9f);
	hi = glm::vec3(-1e9f);
	bool any = false;
	std::string line;
	while (std::getline(file, line))
	{
		if (line.size() < 2 || line[0] != 'v' || line[1] != ' ')
			continue;
		std::istringstream ss(line);
		std::string tag;
		glm::vec3 v;
		ss >> tag >> v.x >> v.y >> v.z;
		lo = glm::min(lo, v);
		hi = glm::max(hi, v);
		any = true;
	}
	return any;
}

void Scene3D::RecomputeModelBounds(Scene3DModel* m) const
{
	if (m == nullptr || !m->hasLocalBounds)
		return;

	// Same transform Scene3DModel::Render uses: T * Ry * Rx * Rz * S. Transform
	// the 8 local corners and take the world-space min/max (a loose AABB).
	glm::mat4 mat(1.0f);
	mat = glm::translate(mat, m->position);
	mat = glm::rotate(mat, glm::radians(m->yawDeg), glm::vec3(0, -1, 0));
	mat = glm::rotate(mat, glm::radians(m->pitchDeg), glm::vec3(1, 0, 0));
	mat = glm::rotate(mat, glm::radians(m->rollDeg), glm::vec3(0, 0, 1));
	mat = glm::scale(mat, m->EffectiveScale());

	glm::vec3 lo(1e9f), hi(-1e9f);
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 c(
			(i & 1) ? m->localMax.x : m->localMin.x,
			(i & 2) ? m->localMax.y : m->localMin.y,
			(i & 4) ? m->localMax.z : m->localMin.z);
		glm::vec3 w = glm::vec3(mat * glm::vec4(c, 1.0f));
		lo = glm::min(lo, w);
		hi = glm::max(hi, w);
	}
	m->aabbMin = lo;
	m->aabbMax = hi;
}

Scene3DModel* Scene3D::PickModel(Game& game, float sx, float sy) const
{
	// Slab ray-vs-AABB against every model's world pick box; return the nearest.
	glm::vec3 ro, rd;
	game.renderer.camera.ScreenPointToRay(sx, sy,
		(float)game.screenWidth, (float)game.screenHeight, ro, rd);

	Scene3DModel* best = nullptr;
	float bestT = 1e30f;
	for (Scene3DModel* m : models)
	{
		const glm::vec3& lo = m->aabbMin;
		const glm::vec3& hi = m->aabbMax;
		float tmin = -1e30f, tmax = 1e30f;
		bool miss = false;
		for (int i = 0; i < 3; i++)
		{
			if (std::fabs(rd[i]) < 1e-8f)
			{
				if (ro[i] < lo[i] || ro[i] > hi[i]) { miss = true; break; }
			}
			else
			{
				float inv = 1.0f / rd[i];
				float t1 = (lo[i] - ro[i]) * inv;
				float t2 = (hi[i] - ro[i]) * inv;
				if (t1 > t2) std::swap(t1, t2);
				tmin = std::max(tmin, t1);
				tmax = std::min(tmax, t2);
				if (tmin > tmax) { miss = true; break; }
			}
		}
		if (miss || tmax < 0.0f)
			continue;
		float t = (tmin >= 0.0f) ? tmin : tmax;
		if (t < bestT)
		{
			bestT = t;
			best = m;
		}
	}
	return best;
}

bool Scene3D::SaveScene(Game& game)
{
	if (currentScene.empty())
		return false;

	std::string path = "data/scenes/" + currentScene + ".scene";
	std::ofstream out(path);
	if (!out.is_open())
	{
		std::cout << "Scene3D: SaveScene cannot write " << path << std::endl;
		return false;
	}

	WriteScene(out);
	out.close();
	std::cout << "Scene3D: saved scene to " << path << std::endl;
	return true;
}

void Scene3D::WriteScene(std::ostream& out) const
{
	out << "# Saved by the in-game 3D editor.\n";
	out << "# model <obj> <tex> <x> <y> <z> <yaw> <scale> [solid] [tag <VALUE>]\n";
	out << "#   optional: rot <pitch> <roll>   scaleaxis <sx> <sy> <sz>\n\n";

	for (Scene3DModel* m : models)
	{
		glm::vec3 p = m->position;
		out << "model " << m->objPath << " " << m->texPath << " "
			<< p.x << " " << p.y << " " << p.z << " "
			<< m->yawDeg << " " << m->modelScale;
		if (m->solid)
			out << " solid";
		if (!m->interactionTag.empty())
			out << " tag " << m->interactionTag;
		if (m->pitchDeg != 0.0f || m->rollDeg != 0.0f)
			out << " rot " << m->pitchDeg << " " << m->rollDeg;
		if (m->scaleAxis.x != 1.0f || m->scaleAxis.y != 1.0f || m->scaleAxis.z != 1.0f)
			out << " scaleaxis " << m->scaleAxis.x << " " << m->scaleAxis.y << " " << m->scaleAxis.z;
		if (!m->materialName.empty())
			out << " mat " << m->materialName;
		if (m->IsWater())
			out << " water " << m->water.amplitude << " " << m->water.waveScale << " "
				<< m->water.shoreFade << " " << m->water.choppy << " " << m->water.specular
				<< " " << m->water.shininess << " " << m->water.opacity;
		out << "\n";
	}
	out << "\n";

	for (Character3D* ch : characters)
	{
		glm::vec3 p = ch->position;
		if (ch->mode == "combined")
		{
			out << "character " << ch->charName << " combined "
				<< ch->spritePath << " "
				<< p.x << " " << p.y << " " << p.z << " " << ch->worldHeight << "\n";
		}
		else
		{
			out << "character " << ch->charName << " layered "
				<< ch->folder << " " << ch->bodyPose << " " << ch->headExpr << " "
				<< p.x << " " << p.y << " " << p.z << " " << ch->worldHeight << "\n";
		}
	}
	out << "\n";

	// Sky + lighting
	if (!skyTexPath.empty())
		out << "sky " << skyTexPath << " " << skyRadiusVal << "\n";
	out << "ambient " << ambientColor.r << " " << ambientColor.g << " "
		<< ambientColor.b << "\n";
	if (dirLight.diffuse > 0.0f)
	{
		out << "light " << dirLight.dir.x << " " << dirLight.dir.y << " "
			<< dirLight.dir.z << " " << dirLight.color.r << " "
			<< dirLight.color.g << " " << dirLight.color.b << " "
			<< dirLight.diffuse << "\n";
	}
	for (const ScenePointLight& p : pointLights)
	{
		out << "point " << p.name << " " << p.pos.x << " " << p.pos.y << " "
			<< p.pos.z << " " << p.color.r << " " << p.color.g << " "
			<< p.color.b << " " << p.range << " " << p.intensity << "\n";
	}
	for (const SceneSpotLight& s : spotLights)
	{
		out << "spot " << s.name << " " << s.pos.x << " " << s.pos.y << " "
			<< s.pos.z << " " << s.dir.x << " " << s.dir.y << " " << s.dir.z
			<< " " << s.color.r << " " << s.color.g << " " << s.color.b
			<< " " << s.range << " " << s.intensity << " " << s.innerDeg
			<< " " << s.outerDeg << "\n";
	}
	// Which point light casts shadows (empty = auto-pick the strongest).
	if (!shadowCasterLight.empty())
		out << "shadowlight " << shadowCasterLight << "\n";
	out << "\n";

	// Cameras (preserve load order; the first is the default view)
	for (const std::string& name : cameraOrder)
	{
		auto it = cameras.find(name);
		if (it == cameras.end())
			continue;
		const CamPose& c = it->second;
		out << "camera " << name << " " << c.position.x << " " << c.position.y
			<< " " << c.position.z << " " << c.pitch << " " << c.yaw << "\n";
	}

}

std::string Scene3D::SerializeToString() const
{
	std::ostringstream ss;
	WriteScene(ss);
	return ss.str();
}


bool Scene3D::Reload(Game& game)
{
	if (currentScene.empty())
		return false;
	std::string name = currentScene;  // Load may clear currentScene mid-call
	return Load(game, name);
}

std::vector<std::string> Scene3D::GetSceneList() const
{
	std::vector<std::string> out;
	try
	{
		namespace fs = std::filesystem;
		for (const auto& e : fs::directory_iterator("data/scenes"))
			if (e.is_regular_file() && e.path().extension() == ".scene")
				out.push_back(e.path().stem().generic_string());
	}
	catch (const std::exception& ex)
	{
		std::cout << "Scene3D: scene list scan failed (" << ex.what() << ")" << std::endl;
	}
	std::sort(out.begin(), out.end());
	return out;
}

bool Scene3D::NewScene(Game& game, const std::string& name)
{
	if (name.empty())
		return false;

	std::string path = "data/scenes/" + name + ".scene";
	namespace fs = std::filesystem;
	if (!fs::exists(path))
	{
		std::ofstream out(path);
		if (!out.is_open())
		{
			std::cout << "Scene3D: NewScene cannot create " << path << std::endl;
			return false;
		}
		// Minimal starter: a fill light and one camera so the empty scene is
		// visible and navigable. The user adds models via the editor's ADD.
		out << "# New scene created by the 3D editor\n";
		out << "ambient 0.30 0.30 0.35\n";
		out << "camera main 0 -200 400 -15 90\n";
		out.close();
		std::cout << "Scene3D: created new scene " << path << std::endl;
	}
	return Load(game, name);
}

void Scene3D::RebuildSolids()
{
	solids.clear();
	for (Scene3DModel* m : models)
	{
		if (!m->solid)
			continue;
		SolidBox box;
		if (ComputeSolidBox(m->objPath, m->position, m->yawDeg, m->modelScale, box))
			solids.push_back(box);
	}
}

std::vector<Scene3D::ModelDef> Scene3D::GetModelPalette() const
{
	std::vector<ModelDef> out;
	auto has = [&](const std::string& obj) {
		for (const ModelDef& d : out) if (d.obj == obj) return true;
		return false;
	};

	// Intended obj -> (tex, solid) pairings from the models already placed.
	std::map<std::string, ModelDef> known;
	std::string folder = "assets/models/basement";
	std::string fallbackTex;
	for (Scene3DModel* m : models)
	{
		known[m->objPath] = { m->objPath, m->texPath, m->solid };
		if (fallbackTex.empty()) fallbackTex = m->texPath;
		size_t slash = m->objPath.find_last_of("/\\");
		if (slash != std::string::npos) folder = m->objPath.substr(0, slash);
	}

	// Every .obj in the scene's model folder becomes an addable type.
	try
	{
		namespace fs = std::filesystem;
		for (const auto& e : fs::directory_iterator(folder))
		{
			if (!e.is_regular_file() || e.path().extension() != ".obj")
				continue;
			std::string obj = e.path().generic_string();
			if (has(obj))
				continue;

			ModelDef def;
			def.obj = obj;
			auto it = known.find(obj);
			if (it != known.end())
			{
				// The scene already uses this mesh: keep its exact pairing.
				def.tex = it->second.tex;
				def.solid = it->second.solid;
			}
			else
			{
				// Not placed in this scene (e.g. a brand-new empty scene): use
				// the known default pairing for the mesh, else a same-basename
				// texture, else any texture we have.
				static const std::map<std::string, std::pair<std::string, bool>> kDefaults = {
					{ "floor",   { "concrete", false } }, { "ceiling", { "ceiling", false } },
					{ "walls",   { "wall",     false } }, { "desk",    { "wood",    true  } },
					{ "monitor", { "screen",   false } }, { "chair",   { "metal",   true  } },
					{ "couch",   { "couch",    true  } }, { "stairs",  { "step",    true  } },
				};
				std::string stem = e.path().stem().generic_string();
				auto dt = kDefaults.find(stem);
				if (dt != kDefaults.end())
				{
					def.tex = folder + "/" + dt->second.first + ".png";
					def.solid = dt->second.second;
				}
				else
				{
					std::string png = folder + "/" + stem + ".png";
					def.tex = fs::exists(png) ? png : fallbackTex;
					def.solid = false;
				}
			}
			out.push_back(def);
		}
	}
	catch (const std::exception& ex)
	{
		std::cout << "Scene3D: palette scan failed (" << ex.what()
			<< "), using placed models only" << std::endl;
	}

	// Fallback: if the scan found nothing, offer the distinct placed models.
	if (out.empty())
		for (auto& kv : known)
			if (!has(kv.first)) out.push_back(kv.second);

	std::sort(out.begin(), out.end(),
		[](const ModelDef& a, const ModelDef& b) { return a.obj < b.obj; });
	return out;
}

Scene3DModel* Scene3D::AddModelInstance(Game& game, const ModelDef& def, const glm::vec3& pos)
{
	if (shader == nullptr)   // scene not loaded yet
		return nullptr;

	Scene3DModel* m = new Scene3DModel(pos);
	m->yawDeg = 0.0f;
	m->modelScale = 1.0f;
	m->shader = shader;
	m->texture = game.spriteManager.GetImage(def.tex, Texture::Filter::Smooth);
	m->objPath = def.obj;
	m->texPath = def.tex;
	m->solid = def.solid;
#ifdef USE_ASSIMP
	m->model3D.LoadModel(def.obj);
	m->loaded = !m->model3D.meshList.empty();
#endif
	m->hasLocalBounds = ReadObjLocalAABB(def.obj, m->localMin, m->localMax);
	RecomputeModelBounds(m);

	models.push_back(m);
	game.entities.push_back(m);
	if (m->solid)
		RebuildSolids();

	std::cout << "Scene3D: added " << def.obj << std::endl;
	return m;
}

bool Scene3D::RemoveModel(Game& game, int index)
{
	if (index < 0 || index >= (int)models.size())
		return false;
	Scene3DModel* m = models[index];
	models.erase(models.begin() + index);
	game.ShouldDeleteEntity(m);   // engine removes it from entities + frees it
	RebuildSolids();
	std::cout << "Scene3D: removed model " << index << std::endl;
	return true;
}

bool Scene3D::RemoveCharacter(Game& game, int index)
{
	if (index < 0 || index >= (int)characters.size())
		return false;
	Character3D* c = characters[index];
	characters.erase(characters.begin() + index);
	game.ShouldDeleteEntity(c);
	std::cout << "Scene3D: removed character " << index << std::endl;
	return true;
}

bool Scene3D::ComputeSolidBox(const std::string& objPath, const glm::vec3& pos,
	float yawDeg, float scale, SolidBox& out) const
{
	// Local-space AABB from the OBJ's vertex positions
	glm::vec3 lo, hi;
	if (!ReadObjLocalAABB(objPath, lo, hi))
	{
		std::cout << "Scene3D: solid - no verts in " << objPath << std::endl;
		return false;
	}

	// Same transform Scene3DModel::Render uses: T * Ry(yaw) * S. Transform
	// all 8 local corners and take the world XZ min/max (a yaw-loose AABB).
	glm::mat4 m(1.0f);
	m = glm::translate(m, pos);
	m = glm::rotate(m, glm::radians(yawDeg), glm::vec3(0, -1, 0));
	m = glm::scale(m, glm::vec3(scale));

	out.minX = out.minZ = 1e9f;
	out.maxX = out.maxZ = -1e9f;
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 c(
			(i & 1) ? hi.x : lo.x,
			(i & 2) ? hi.y : lo.y,
			(i & 4) ? hi.z : lo.z);
		glm::vec3 w = glm::vec3(m * glm::vec4(c, 1.0f));
		out.minX = glm::min(out.minX, w.x);
		out.maxX = glm::max(out.maxX, w.x);
		out.minZ = glm::min(out.minZ, w.z);
		out.maxZ = glm::max(out.maxZ, w.z);
	}
	return true;
}

void Scene3D::ResolveAgainstSolids(glm::vec3& pos, float radius) const
{
	if (solids.empty())
		return;

	// Iterate: pushing out of one box can push into another, so repeat until
	// clear (or a safety cap). Each step nudges out of the box the point is
	// deepest in, along the axis of least penetration.
	for (int iter = 0; iter < 8; iter++)
	{
		bool moved = false;
		for (const SolidBox& b : solids)
		{
			float minX = b.minX - radius, maxX = b.maxX + radius;
			float minZ = b.minZ - radius, maxZ = b.maxZ + radius;
			if (pos.x <= minX || pos.x >= maxX || pos.z <= minZ || pos.z >= maxZ)
				continue;  // outside this (expanded) box

			// Penetration depth toward each of the 4 edges; exit the nearest
			float penL = pos.x - minX;   // push -x
			float penR = maxX - pos.x;   // push +x
			float penD = pos.z - minZ;   // push -z
			float penU = maxZ - pos.z;   // push +z
			float m = glm::min(glm::min(penL, penR), glm::min(penD, penU));

			if (m == penL)      pos.x = minX;
			else if (m == penR) pos.x = maxX;
			else if (m == penD) pos.z = minZ;
			else                pos.z = maxZ;
			moved = true;
		}
		if (!moved)
			break;
	}
}

// --- runtime light control (find the named light in point then spot) ---

bool Scene3D::SetLightOn(const std::string& name, bool on)
{
	for (ScenePointLight& p : pointLights)
		if (p.name == name) { p.on = on; p.fade.active = false; return true; }
	for (SceneSpotLight& s : spotLights)
		if (s.name == name) { s.on = on; s.fade.active = false; return true; }
	std::cout << "Scene3D: no light named '" << name << "'" << std::endl;
	return false;
}

bool Scene3D::SetLightIntensity(const std::string& name, float intensity)
{
	for (ScenePointLight& p : pointLights)
		if (p.name == name) { p.intensity = intensity; p.on = true; p.fade.active = false; return true; }
	for (SceneSpotLight& s : spotLights)
		if (s.name == name) { s.intensity = intensity; s.on = true; s.fade.active = false; return true; }
	std::cout << "Scene3D: no light named '" << name << "'" << std::endl;
	return false;
}

bool Scene3D::FadeLightIntensity(const std::string& name, float target, float seconds)
{
	auto startFade = [&](float& intensity, bool& on, LightFade& f)
	{
		on = true;  // fading implies the light participates (even from 0)
		f.active = true;
		f.from = intensity;
		f.to = target;
		f.elapsed = 0.0f;
		f.duration = (seconds > 0.001f) ? seconds : 0.001f;
	};
	for (ScenePointLight& p : pointLights)
		if (p.name == name) { startFade(p.intensity, p.on, p.fade); return true; }
	for (SceneSpotLight& s : spotLights)
		if (s.name == name) { startFade(s.intensity, s.on, s.fade); return true; }
	std::cout << "Scene3D: no light named '" << name << "'" << std::endl;
	return false;
}

bool Scene3D::SetLightColor(const std::string& name, const glm::vec3& color)
{
	for (ScenePointLight& p : pointLights)
		if (p.name == name) { p.color = color; return true; }
	for (SceneSpotLight& s : spotLights)
		if (s.name == name) { s.color = color; return true; }
	std::cout << "Scene3D: no light named '" << name << "'" << std::endl;
	return false;
}

bool Scene3D::SetLightPosition(const std::string& name, const glm::vec3& pos)
{
	for (ScenePointLight& p : pointLights)
		if (p.name == name) { p.pos = pos; return true; }
	for (SceneSpotLight& s : spotLights)
		if (s.name == name) { s.pos = pos; return true; }
	std::cout << "Scene3D: no light named '" << name << "'" << std::endl;
	return false;
}

void Scene3D::SetSkyTint(const glm::vec3& tint)
{
	if (skybox == nullptr || skybox->GetSprite() == nullptr)
		return;
	auto ch = [](float v) {
		int x = (int)(v * 255.0f + 0.5f);
		return (Uint8)(x < 0 ? 0 : (x > 255 ? 255 : x));
	};
	skybox->GetSprite()->color = Color{ ch(tint.r), ch(tint.g), ch(tint.b), 255 };
}

void Scene3D::SetSkyTexture(Game& game, const std::string& path)
{
	// Swap the panorama in place. Callers dedupe (only call on a real change);
	// skyTexPath is left untouched so a scene save keeps the authored sky.
	if (skybox == nullptr || skybox->GetSprite() == nullptr || path.empty())
		return;
	Texture* tex = game.spriteManager.GetImage(path);
	if (tex == nullptr)
		return;
	Sprite* s = skybox->GetSprite();
	s->SetTexture(tex);
	s->frameWidth = tex->GetWidth();
	s->frameHeight = tex->GetHeight();
	skybox->nextTexture = nullptr;   // a hard set clears any cross-fade
	skybox->blendToNext = 0.0f;
}

void Scene3D::SetSkyCrossfade(Game& game, const std::string& fromPath,
	const std::string& toPath, float blend)
{
	if (skybox == nullptr || skybox->GetSprite() == nullptr)
		return;
	Sprite* s = skybox->GetSprite();

	Texture* a = game.spriteManager.GetImage(fromPath);   // cached lookup
	if (a != nullptr && s->texture != a)
	{
		s->texture = a;
		s->frameWidth = a->GetWidth();
		s->frameHeight = a->GetHeight();
	}

	Texture* b = toPath.empty() ? nullptr : game.spriteManager.GetImage(toPath);
	skybox->nextTexture = (b != nullptr && b != s->texture) ? b : nullptr;
	skybox->blendToNext = (skybox->nextTexture != nullptr) ? blend : 0.0f;
}

void Scene3D::Unload(Game& game)
{
	if (!active)
		return;

	for (Scene3DModel* m : models)
	{
		game.ShouldDeleteEntity(m);
	}
	for (Character3D* ch : characters)
	{
		game.ShouldDeleteEntity(ch);
	}
	if (skybox != nullptr)
	{
		game.ShouldDeleteEntity(skybox);
		skybox = nullptr;
	}
	models.clear();
	characters.clear();
	solids.clear();
	cameras.clear();
	cameraOrder.clear();
	active = false;
	gliding = false;
	currentScene.clear();

	RestoreOrtho(game);
	std::cout << "Scene3D: unloaded, back to 2D" << std::endl;
}

void Scene3D::GlideToPose(const CamPose& pose, float seconds)
{
	// glideFrom is captured from the live camera on the first Update tick,
	// so a glide issued mid-glide starts from the current interpolated pose
	glideTo = pose;
	glideSeconds = (seconds > 0.01f) ? seconds : 0.9f;
	glideBlend = 0.0f;
	gliding = true;
	glideFromCaptured = false;
}

bool Scene3D::GlideToCamera(const std::string& camName, float seconds)
{
	auto it = cameras.find(camName);
	if (it == cameras.end())
	{
		std::cout << "Scene3D: unknown camera '" << camName << "'" << std::endl;
		return false;
	}
	GlideToPose(it->second, seconds);
	std::cout << "Scene3D: gliding to '" << camName << "' over " << glideSeconds << "s" << std::endl;
	return true;
}

Character3D* Scene3D::FindCharacter(const std::string& name) const
{
	for (Character3D* ch : characters)
		if (ch != nullptr && ch->charName == name)
			return ch;
	return nullptr;
}

bool Scene3D::FocusCharacter(Game& game, const std::string& charName,
	float seconds, bool closeup, float distanceOverride)
{
	Character3D* ch = FindCharacter(charName);
	if (ch == nullptr)
	{
		std::cout << "Scene3D: no character named '" << charName << "'" << std::endl;
		return false;
	}

	// Decide the vertical WORLD span to fit and where to look, as fractions
	// of the billboard height measured from the base (feet=0, up=1).
	float frameBotFrac, frameTopFrac;
	if (closeup)
	{
		// Upper-body dialogue shot: from the waist up to just above the head.
		// Waist sits ~45% up the visible figure; a little headroom above the
		// head keeps the top of the head inside the frame.
		float figH = ch->figureTopFrac - ch->figureBotFrac;
		float waist = ch->figureBotFrac + 0.45f * figH;
		float headroom = 0.06f * figH;
		frameBotFrac = waist;
		frameTopFrac = ch->figureTopFrac + headroom;
	}
	else
	{
		// Whole figure with a touch of margin
		float figH = ch->figureTopFrac - ch->figureBotFrac;
		frameBotFrac = ch->figureBotFrac - 0.04f * figH;
		frameTopFrac = ch->figureTopFrac + 0.04f * figH;
	}

	// Vertical look-at height = the middle of the framed span (upper chest
	// for a closeup).
	float centerFrac = 0.5f * (frameBotFrac + frameTopFrac);
	glm::vec3 target = ch->position + glm::vec3(0.0f, -centerFrac * ch->worldHeight, 0.0f);

	// Keep the horizontal side the camera is currently on (dolly toward the
	// character rather than teleporting around them)
	glm::vec3 camPos = game.renderer.camera.position;
	glm::vec3 sideXZ(camPos.x - target.x, 0.0f, camPos.z - target.z);
	if (glm::length(sideXZ) < 0.001f)
		sideXZ = glm::vec3(0, 0, 1);
	sideXZ = glm::normalize(sideXZ);

	// Horizontal centering on the DRAWN body: the billboard's opaque art may
	// be offset within its canvas, so shift the look-at along the billboard's
	// right axis by (hcenter - 0.5) of its width. right = cross(toCam, up)
	// matches Character3D::DrawQuad (toCam == sideXZ at the framed position;
	// visual up is world -Y). u=0..1 maps to local x -0.5..0.5, so a body
	// whose opaque center is at canvas fraction h lands at local x (h-0.5).
	{
		float aspect = (ch->bodyTex != nullptr && ch->bodyTex->GetHeight() > 0)
			? (float)ch->bodyTex->GetWidth() / (float)ch->bodyTex->GetHeight() : 0.5f;
		float billboardWidth = ch->worldHeight * aspect;
		glm::vec3 right = glm::normalize(glm::cross(sideXZ, glm::vec3(0.0f, -1.0f, 0.0f)));
		float hOffset = (ch->figureHCenterFrac - 0.5f) * billboardWidth;
		target += right * hOffset;
	}

	// Dolly along the view axis to fit the framed span in the vertical FOV
	float spanWorld = (frameTopFrac - frameBotFrac) * ch->worldHeight;
	float dist = distanceOverride > 0.01f
		? distanceOverride
		: (spanWorld * 0.5f) / tanf(glm::radians(perspFovDeg * 0.5f));

	// Level, eye-line shot at the target's height
	CamPose pose;
	pose.position = target + sideXZ * dist;
	pose.position.y = target.y;

	// Aim at the target: the engine looks at (position - front), so
	// front = normalize(position - target); derive yaw/pitch from it
	glm::vec3 front = glm::normalize(pose.position - target);
	pose.pitch = glm::degrees(asinf(glm::clamp(front.y, -1.0f, 1.0f)));
	pose.yaw = glm::degrees(atan2f(front.z, front.x));

	GlideToPose(pose, seconds);
	focusCharName = charName;  // centering/framing verified when the glide settles
	focusTarget = target;
	focusHeadFrac = ch->figureTopFrac;
	std::cout << "Scene3D: " << (closeup ? "closeup" : "focus") << " '" << charName
		<< "' over " << glideSeconds << "s, dist " << dist
		<< ", pose yaw " << pose.yaw << " pitch " << pose.pitch << std::endl;
	return true;
}

void Scene3D::Update(Game& game)
{
	// Interactive edit launch: load the requested scene once (the game is
	// fully initialized by the time gui->Update() first runs this).
	if (!autoLoadScene.empty())
	{
		std::string s = autoLoadScene;
		autoLoadScene.clear();
		Load(game, s);
	}

	// Test harness: once the test cutscene has ended (autoreturn set
	// watchingCutscene false), close the game. Gated on a scene having
	// loaded so we never quit before the test actually runs.
	if (testAutoQuit && sceneEverLoaded && !game.cutsceneManager.watchingCutscene)
	{
		game.shouldQuit = true;
		return;
	}

	if (!active)
		return;

	float dtSec = game.dt / 1000.0f;

	// Advance any light-intensity fades (smoothstep to the target)
	auto stepFade = [dtSec](float& intensity, bool& on, LightFade& f)
	{
		if (!f.active) return;
		f.elapsed += dtSec;
		float t = f.elapsed / f.duration;
		if (t >= 1.0f) t = 1.0f;
		float s = t * t * (3.0f - 2.0f * t);
		intensity = f.from + (f.to - f.from) * s;
		if (t >= 1.0f)
		{
			f.active = false;
			if (intensity <= 0.0001f) on = false;  // faded fully out -> disable
		}
	};
	for (ScenePointLight& p : pointLights) stepFade(p.intensity, p.on, p.fade);
	for (SceneSpotLight& s : spotLights)   stepFade(s.intensity, s.on, s.fade);

	// Advance an in-progress camera glide
	if (gliding)
	{
		Camera& cam = game.renderer.camera;

		// Capture the starting pose on the first tick (the live camera pose)
		if (!glideFromCaptured)
		{
			glideFrom.position = cam.position;
			glideFrom.pitch = cam.pitch;
			glideFrom.yaw = cam.yaw;
			glideFromCaptured = true;
		}

		glideBlend += dtSec / glideSeconds;
		if (glideBlend >= 1.0f)
			glideBlend = 1.0f;

		float s = glideBlend * glideBlend * (3.0f - 2.0f * glideBlend);  // smoothstep

		// Shortest-path yaw so a 350->10 degree move goes +20, not -340
		float yawDelta = fmodf(glideTo.yaw - glideFrom.yaw + 540.0f, 360.0f) - 180.0f;

		cam.position = glideFrom.position + (glideTo.position - glideFrom.position) * s;
		cam.pitch = glideFrom.pitch + (glideTo.pitch - glideFrom.pitch) * s;
		cam.yaw = glideFrom.yaw + yawDelta * s;
		cam.shouldUpdate = true;
		cam.Update();

		if (glideBlend >= 1.0f)
		{
			gliding = false;
			std::cout << "Scene3D: glide complete" << std::endl;

			// Deterministic centering check: project the focused character's
			// center to screen space; a centered focus lands at the middle
			// of the window (independent of screenshot timing)
			if (!focusCharName.empty())
			{
				Character3D* ch = FindCharacter(focusCharName);
				if (ch != nullptr)
				{
					float w = (float)game.screenWidth, h = (float)game.screenHeight;
					// The look-at target should land dead center...
					glm::vec3 tp = cam.WorldToScreenPoint(focusTarget, w, h);
					// ...and the top of the head should be ON-SCREEN in the
					// upper half (so it's framed, not cropped).
					glm::vec3 headWorld = ch->position
						+ glm::vec3(0.0f, -focusHeadFrac * ch->worldHeight, 0.0f);
					glm::vec3 hp = cam.WorldToScreenPoint(headWorld, w, h);
					bool headInFrame = hp.z > 0.0f && hp.y >= 0.0f && hp.y <= h;
					std::cout << "Scene3D: '" << focusCharName << "' target at ("
						<< tp.x << "," << tp.y << ") [center " << w * 0.5f << ","
						<< h * 0.5f << "], head at y=" << hp.y
						<< " in-frame=" << (headInFrame ? "yes" : "NO") << std::endl;
				}
				focusCharName.clear();
			}
		}
	}
}

void Scene3D::AddOrUpdateCamera(const std::string& name, const CamPose& pose)
{
	bool isNew = cameras.find(name) == cameras.end();
	cameras[name] = pose;
	if (isNew)
		cameraOrder.push_back(name);
}

bool Scene3D::GetCameraPose(const std::string& name, CamPose& out) const
{
	auto it = cameras.find(name);
	if (it == cameras.end())
		return false;
	out = it->second;
	return true;
}

bool Scene3D::SetDefaultCamera(const std::string& name)
{
	if (cameras.find(name) == cameras.end())
		return false;
	auto it = std::find(cameraOrder.begin(), cameraOrder.end(), name);
	if (it != cameraOrder.end())
		cameraOrder.erase(it);
	cameraOrder.insert(cameraOrder.begin(), name);
	return true;
}

bool Scene3D::RemoveCamera(const std::string& name)
{
	if (cameraOrder.size() <= 1)
		return false;   // keep at least one camera
	auto it = std::find(cameraOrder.begin(), cameraOrder.end(), name);
	if (it == cameraOrder.end())
		return false;
	cameraOrder.erase(it);
	cameras.erase(name);
	return true;
}

bool Scene3D::JumpToCamera(Game& game, const std::string& camName)
{
	auto it = cameras.find(camName);
	if (it == cameras.end())
	{
		std::cout << "Scene3D: unknown camera '" << camName << "'" << std::endl;
		return false;
	}

	Camera& cam = game.renderer.camera;
	cam.position = it->second.position;
	cam.pitch = it->second.pitch;
	cam.yaw = it->second.yaw;
	cam.shouldUpdate = true;
	cam.Update();

	std::cout << "Scene3D: camera '" << camName << "' pos ("
		<< cam.position.x << "," << cam.position.y << "," << cam.position.z
		<< ") pitch " << cam.pitch << " yaw " << cam.yaw << std::endl;
	return true;
}

void Scene3D::EnterPerspective(Game& game)
{
	Camera& cam = game.renderer.camera;
	saved2DCameraPos = cam.position;

	cam.useOrthoCamera = false;
	cam.SetWorldUp(glm::vec3(0, 1, 0));  // engine Y-up convention (visual up = -Y)
	cam.SetupPerspective(60.0f, 0.1f, 5000.0f);

	game.useDepthTesting = true;
	game.renderer.SetDepthTestEnabled(true);
}

void Scene3D::RestoreOrtho(Game& game)
{
	Camera& cam = game.renderer.camera;

	cam.useOrthoCamera = true;
	cam.position = saved2DCameraPos;
	cam.targetOffset = glm::vec3(0, 0, 0);
	cam.SetWorldUp(glm::vec3(0, 1, 0));
	cam.yaw = 90.0f;
	cam.pitch = 0.0f;
	cam.shouldUpdate = false;
	cam.Update();

	// Rebuild the VN's ortho projection (same expression MazeMiner uses to
	// restore its 2D mode)
	float zoomX = cam.startScreenWidth * cam.orthoZoom;
	float zoomY = cam.startScreenHeight * cam.orthoZoom;
	cam.projection = glm::ortho(0.0f, zoomX, zoomY, 0.0f, -1.0f, 10.0f);

	game.useDepthTesting = false;
	game.renderer.SetDepthTestEnabled(false);
}
