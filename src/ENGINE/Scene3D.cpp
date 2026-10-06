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
#include <functional>
#include <unordered_set>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstddef>
#include "UniformBlocks.h"
#include "UniformBufferCache.h"
#include "RenderState.h"
#include "TransientBuffer.h"

#include "Scene3DInternal.h"
#include "ModelMaterials.h"
#include "render/ColorPipeline.h"
#include "render/Environment.h"
#include "render/AmbientOcclusion.h"
#include "render/TemporalAA.h"
#include "render/ColorGrading.h"
#include "render/DepthOfField.h"
#include "render/VolumetricFog.h"
#include "render/DistanceFog.h"
#include "render/RenderViews.h"
#include "render/Reflections.h"

using Scene3DInternal::ProgramHasBlock;
using Scene3DInternal::SceneColorTexture;

namespace
{
	// Models animating (Scene3D::AnimateModelTurn / AnimateModelMove). Kept out
	// of the exported class, whose layout the DLL's ABI fixes.
	struct ModelTween
	{
		Scene3DModel* model = nullptr;
		bool turn = false;               // yaw (from.x -> to.x), else position
		glm::vec3 from = glm::vec3(0.0f);
		glm::vec3 to = glm::vec3(0.0f);
		float seconds = 0.0f;
		float elapsed = 0.0f;
	};
	std::vector<ModelTween> modelTweens;
}

Texture* Scene3DInternal::SceneColorTexture(const Game& game, const std::string& path)
{
	// "-": no texture of its own (a glTF model brings its materials' maps).
	if (path == "-")
		return ModelWhiteTexture();
	return game.spriteManager.GetImage(path, Texture::Filter::Smooth, LinearWorkflow());
}

void Scene3DInternal::DrawMeshesForDepth(const std::vector<Mesh*>& meshes, Texture* modelTexture)
{
	// A mesh with its own (glTF) material cuts out by its own alpha mode.
	const bool own = HasModelMaterials(meshes);
	if (!own && modelTexture != nullptr)
		modelTexture->UseTexture();
	for (Mesh* mesh : meshes)
	{
		if (own)
		{
			const ModelMaterial* material = MeshMaterial(mesh);
			Texture* alpha = (material != nullptr) ? ModelShadowAlpha(*material) : modelTexture;
			if (alpha != nullptr)
				alpha->UseTexture();
		}
		mesh->RenderMesh(0);
	}
}

// ------------------------------------------------------- motion vectors

namespace
{
	// Each drawn object's position in the last two frames it was drawn in.
	struct MotionTrack
	{
		glm::vec3 prevPos = glm::vec3(0.0f);
		glm::vec3 curPos = glm::vec3(0.0f);
		unsigned int prevFrame = 0;
		unsigned int curFrame = 0;
	};
	std::unordered_map<const void*, MotionTrack> motionTracks;

	// Set while RenderTransparentModels draws: a model whose meshes carry
	// their own (glTF) materials then draws only its blended meshes, and the
	// opaque pass only the others.
	bool drawingTransparent = false;

	// Draw `meshes` with `program` bound and the model's own state applied: a
	// mesh with its own material applies that first, and draws only in its
	// pass (blended meshes in the transparent pass, the rest in the opaque one).
	// With `instances`, each mesh draws once per matrix (an instance group).
	void DrawModelMeshes(unsigned int program, const std::vector<Mesh*>& meshes, bool transparentPass,
		const std::vector<glm::mat4>* instances = nullptr)
	{
		const bool own = HasModelMaterials(meshes);
		for (Mesh* mesh : meshes)
		{
			if (own)
			{
				const ModelMaterial* material = MeshMaterial(mesh);
				if (material == nullptr || (material->alphaMode == AlphaMode::Blend) != transparentPass)
					continue;
				Scene3DInternal::ApplyModelMaterial(program, *material);
			}
			if (instances != nullptr)
				mesh->SetInstancesTransient(instances->data(), (unsigned int)instances->size());
			mesh->RenderMesh(0);
			if (instances != nullptr)
				mesh->ClearInstances();   // restore pristine VAO for the shadow/other passes
		}
	}
}

unsigned int Scene3DInternal::WorldDrawBuffers(unsigned int program, bool characterMask)
{
	unsigned int buffers = 1u;
	if (characterMask)
		buffers |= 2u;
	if (MotionWritesActive() && ProgramHasBlock(program, "Motion"))
		buffers |= 4u;
	return buffers;
}

glm::vec3 Scene3DInternal::MotionOffset(const void* object, const glm::vec3& position)
{
	const unsigned int frame = TemporalFrameIndex();
	if (frame == 0)
		return glm::vec3(0.0f);
	if (motionTracks.size() > 8192)
		motionTracks.clear();   // deleted objects' entries pile up; a cleared track only costs one frame
	MotionTrack& t = motionTracks[object];
	if (t.curFrame != frame)
	{
		t.prevPos = t.curPos;
		t.prevFrame = t.curFrame;
		t.curPos = position;
		t.curFrame = frame;
	}
	return (t.prevFrame != 0 && t.prevFrame + 1 == frame) ? position - t.prevPos : glm::vec3(0.0f);
}

void Scene3DInternal::ForgetMotion()
{
	motionTracks.clear();
}

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

glm::mat4 Scene3DModel::ModelMatrix() const
{
	glm::mat4 model(1.0f);
	model = glm::translate(model, position);
	// Yaw about vertical (-Y), then pitch about X, then roll about Z.
	model = glm::rotate(model, glm::radians(yawDeg), glm::vec3(0, -1, 0));
	model = glm::rotate(model, glm::radians(pitchDeg), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians(rollDeg), glm::vec3(0, 0, 1));
	model = glm::scale(model, EffectiveScale());
	return model;
}

void Scene3DModel::Render(const Renderer& renderer)
{
	if (guardHidden)   // availability guard says this object isn't present now
		return;
	// GPU-driven models (Scene3DGpuDriven.cpp): the first to Render draws them all.
	if (Scene3DInternal::GpuDrawnColour(this))
	{
		Scene3D::Get().DrawGpuDrivenModels(renderer);
		return;
	}
	// Instanced grouping (rebuilt each frame): duplicate opaque props draw once,
	// from their group leader; the other members skip this pass.
	if (instanceMember)
		return;
	if (instanceGroupIndex >= 0)
	{
		Scene3D::Get().DrawInstancedGroup(renderer, instanceGroupIndex);
		return;
	}
	// Transparent models (material opacity < 1) are drawn later, back-to-front,
	// in Scene3D::RenderTransparentModels - skip them in the normal opaque pass.
	if (material != nullptr && material->IsTransparent() && !HasModelMaterials(model3D.meshList))
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

	glm::mat4 model = ModelMatrix();
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

	Scene3D& scene = Scene3D::Get();
	const SceneMaterial& mat = material ? *material : MaterialLibrary::Get().Default();

	shader->UseShader();
	renderer.BindWorldCameraBlock();   // view/projection: Camera block
	unsigned int id = shader->GetID();
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), model);
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));

	// normalMatrix (computed above) corrects normals under non-uniform scale.
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "normalMatrix")), normalMatrix);

	// Material block (mat resolved above; default matte material as fallback).
	// Water models pass their per-surface tuning, which overrides the
	// material's specular/shininess/opacity so lakes sharing one water
	// material can still be tuned individually in the editor.
	scene.ApplyMaterial(id, mat, &water);
	scene.ApplyLighting(id, renderer);

	texture->UseTexture();

	// Motion vectors (temporal anti-aliasing): this draw also writes how far
	// the model moved since last frame.
	const unsigned int buffers = Scene3DInternal::WorldDrawBuffers(id, false);
	const glm::vec3 motion = (buffers & 4u) ? Scene3DInternal::MotionOffset(this, position) : glm::vec3(0.0f);
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "motionOffset")), motion);
	if (buffers != 1u)
		Device().SetBoundDrawBufferMask(buffers);

	DrawModelMeshes(id, model3D.meshList, drawingTransparent);

	if (buffers != 1u)
		Device().SetBoundDrawBufferMask(1u);
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
	{
		if (m->guardHidden)
			continue;
		// glTF: models with blended meshes (only those meshes draw here).
		const bool own = HasModelMaterials(m->model3D.meshList);
		if (own ? HasBlendedModelMaterial(m->model3D.meshList)
			: (m->material != nullptr && m->material->IsTransparent()))
			transparent.push_back(m);
	}
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

	RenderState glass = CurrentRenderState();
	glass.depthWrite = false;   // don't write depth; keep depth TEST on
	ScopedRenderState scope(glass);
	drawingTransparent = true;
	for (Scene3DModel* m : transparent)
		m->DrawGeometry(renderer);
	drawingTransparent = false;
}

// ------------------------------------------------- ambient occlusion prepass

bool Scene3D::WantsScreenSpacePrepass(const Renderer& renderer) const
{
#ifdef USE_ASSIMP
	// For ambient occlusion and/or screen-space reflections. An older shader
	// copy (gamma mode keeps a game's own) reads neither.
	return active && !renderer.camera.useOrthoCamera && shader != nullptr && PrepassWanted()
		&& ProgramHasBlock(shader->GetID(), "AmbientOcclusion");
#else
	(void)renderer;
	return false;
#endif
}

// The opaque models exactly as the world pass draws them (same skips, same
// instance groups, same alpha cut-outs), into the occlusion targets.
void Scene3D::RenderAoPrepass(Game& game, const Renderer& renderer)
{
#ifdef USE_ASSIMP
	unsigned int program = 0, instancedProgram = 0;
	if (!BeginAoPrepass(ViewTargetWidth(game), ViewTargetHeight(game), program, instancedProgram))
		return;
	RenderDevice& device = Device();
	renderer.BindWorldCameraBlock();

	device.UseProgram(ProgramHandle(program));
	const int modelLoc = ShaderProgram::DrawUniformLocation(program, "model");
	device.SetUniform(device.UniformLocation(ProgramHandle(program), "theTexture"), 0);
	// (Models the GPU-driven path draws are drawn below, GPU-culled.)
	for (Scene3DModel* m : Scene3DInternal::CpuColourModels(models))
	{
		if (m == nullptr || !m->loaded || m->shader == nullptr || m->texture == nullptr || m->guardHidden
			|| m->instanceMember || m->instanceGroupIndex >= 0 || m->IsWater() || !m->active)
			continue;
		const bool own = HasModelMaterials(m->model3D.meshList);
		if (m->material != nullptr && m->material->IsTransparent() && !own)
			continue;
		device.SetUniform(modelLoc, m->ModelMatrix());
		ApplyMaterial(program, m->material ? *m->material : MaterialLibrary::Get().Default(), nullptr);
		m->texture->UseTexture();
		DrawModelMeshes(program, m->model3D.meshList, false);
	}

	// Instance groups (built for this frame by UpdateCameraUBO): one draw each.
	bool instancedBound = false;
	for (const std::vector<Scene3DModel*>& group : instanceGroups)
	{
		if (group.empty())
			continue;
		Scene3DModel* leader = group[0];
		if (!leader->loaded || leader->texture == nullptr || leader->model3D.meshList.empty() || !leader->active
			|| instancedShader == nullptr)
			continue;
		if (!instancedBound)
		{
			device.UseProgram(ProgramHandle(instancedProgram));
			device.SetUniform(device.UniformLocation(ProgramHandle(instancedProgram), "theTexture"), 0);
			instancedBound = true;
		}
		std::vector<glm::mat4> mats;
		mats.reserve(group.size());
		for (Scene3DModel* m : group)
			mats.push_back(m->ModelMatrix());
		ApplyMaterial(instancedProgram, leader->material ? *leader->material : MaterialLibrary::Get().Default(), nullptr);
		leader->texture->UseTexture();
		DrawModelMeshes(instancedProgram, leader->model3D.meshList, false, &mats);
	}

	// GPU-driven models (Scene3DGpuDriven.cpp).
	if (Scene3DInternal::GpuDrivenFrame())
		DrawGpuColourView(renderer, Scene3DInternal::GpuPrepassProgram(), false);

	EndAoPrepass(ViewTargetWidth(game), ViewTargetHeight(game));
#else
	(void)game;
	(void)renderer;
#endif
}

// ----------------------------------------------------- seasonal foliage

void Scene3D::SetSeason(Game& game, Season s)
{
	season = s;
	const char* suffix = (s == Season::Spring) ? "_spring"
		: (s == Season::Autumn) ? "_autumn"
		: (s == Season::Winter) ? "_winter" : "";   // Summer = base texture

	for (Scene3DModel* m : models)
	{
		if (m == nullptr || m->material == nullptr || !m->material->seasonal)
			continue;

		std::string path = m->texPath;   // base = summer / default look
		if (s != Season::Summer && suffix[0] != '\0')
		{
			// Insert the season suffix before the extension: grass.png -> grass_autumn.png
			size_t dot = m->texPath.find_last_of('.');
			std::string variant = (dot == std::string::npos)
				? (m->texPath + suffix)
				: (m->texPath.substr(0, dot) + suffix + m->texPath.substr(dot));
			// Only swap if that variant actually exists (else keep the base look -
			// e.g. evergreen pines have only a winter variant). GetImage would
			// otherwise substitute a white placeholder for a missing file.
			std::ifstream f(variant);
			if (f.good())
				path = variant;
		}
		Texture* t = SceneColorTexture(game, path);
		if (t != nullptr)
			m->texture = t;

		// Deciduous trees shed their leaves: in winter swap the canopy MESH to a
		// bare-branch variant ("<obj>_bare.obj"); any other season restores the
		// authored (leafy) mesh. objPath is left as the authored summer mesh so the
		// .scene file still serializes the leafy tree.
		if (m->material->deciduous)
		{
			bool wantBare = (s == Season::Winter);
			if (wantBare != m->bareMesh)
			{
				std::string meshPath = m->objPath;   // restore target (leafy)
				bool toBare = false;
				if (wantBare)
				{
					size_t d = m->objPath.find_last_of('.');
					std::string bare = (d == std::string::npos)
						? (m->objPath + "_bare")
						: (m->objPath.substr(0, d) + "_bare" + m->objPath.substr(d));
					std::ifstream bf(bare);
					if (bf.good()) { meshPath = bare; toBare = true; }
				}
				// Only reload when we actually have a mesh change to make (skip if
				// winter was requested but no _bare variant exists - keep the leafy
				// mesh, texture already switched to the frosty winter variant).
				if (toBare || !wantBare)
				{
#ifdef USE_ASSIMP
					for (Mesh* mesh : m->model3D.meshList)
						if (mesh != nullptr) delete_it(mesh);
					m->model3D.meshList.clear();
					m->model3D.LoadModel(meshPath);
					m->loaded = !m->model3D.meshList.empty();
#endif
					m->hasLocalBounds = ReadObjLocalAABB(meshPath, m->localMin, m->localMax);
					RecomputeModelBounds(m);
					m->bareMesh = toBare;
				}
			}
		}
	}
}

// ------------------------------------------------- toon outline shader

// Built on first use rather than only by Load - see the note in Scene3D.h.
ShaderProgram* Scene3D::EdgeShader()
{
	if (edgeShader == nullptr)
	{
		edgeShader = new ShaderProgram(-1, "data/shaders/scene3d_edge.vert",
			"data/shaders/scene3d_edge.frag");
	}
	return edgeShader;
}

// ---------------------------------------------------- shared camera block

void Scene3D::UpdateCameraUBO(const Renderer& renderer)
{
	// This frame's GPU-driven model list (or none: it decides, ortho frames too).
	BuildGpuDrawList(renderer);
	if (renderer.camera.useOrthoCamera)
		return;                     // 2D frames don't use the 3D camera block
	// The Camera block itself is the renderer's now (Renderer::BindCameraBlock);
	// every Scene3D draw binds the world camera before drawing, so nothing here
	// relies on binding point 0 staying put between draws.
	renderer.BindWorldCameraBlock();

	RebuildInstanceGroups();
}

// ---------------------------------------------------- instanced opaque props

void Scene3D::RebuildInstanceGroups()
{
	instanceGroups.clear();
	for (Scene3DModel* m : models)
	{
		if (m == nullptr) continue;
		m->instanceGroupIndex = -1;
		m->instanceMember = false;
	}
	if (!instancingEnabled || Scene3DInternal::GpuDrivenFrame())
		return;   // (the GPU-driven path batches every model itself)

	// Bucket opaque, non-water, loaded props that share geometry + texture +
	// material. Only groups of 2+ are worth an instanced draw.
	std::map<std::tuple<std::string, std::string, const SceneMaterial*>,
		std::vector<Scene3DModel*>> buckets;
	for (Scene3DModel* m : models)
	{
		if (m == nullptr || !m->loaded || m->texture == nullptr) continue;
		if (m->guardHidden) continue;   // availability guard: not present -> not drawn
		if (m->IsWater()) continue;
		if (m->material != nullptr && m->material->IsTransparent()) continue;
		if (m->model3D.meshList.empty()) continue;
		buckets[std::make_tuple(m->objPath, m->texPath, m->material)].push_back(m);
	}
	for (auto& kv : buckets)
	{
		if (kv.second.size() < 2)
			continue;                       // singleton: renders normally
		int idx = (int)instanceGroups.size();
		instanceGroups.push_back(kv.second);
		kv.second[0]->instanceGroupIndex = idx;   // leader draws the whole group
		for (size_t i = 1; i < kv.second.size(); i++)
			kv.second[i]->instanceMember = true;  // members skip their own draw
	}
}

void Scene3D::DrawInstancedGroup(const Renderer& renderer, int groupIndex)
{
#ifdef USE_ASSIMP
	if (groupIndex < 0 || groupIndex >= (int)instanceGroups.size())
		return;
	const std::vector<Scene3DModel*>& group = instanceGroups[groupIndex];
	if (group.empty() || instancedShader == nullptr)
		return;
	Scene3DModel* leader = group[0];
	if (!leader->loaded || leader->texture == nullptr || leader->model3D.meshList.empty()
		|| renderer.camera.useOrthoCamera)
		return;

	// Per-instance model matrices (rebuilt each frame so editor moves show up).
	std::vector<glm::mat4> mats;
	mats.reserve(group.size());
	for (Scene3DModel* m : group)
		mats.push_back(m->ModelMatrix());

	const SceneMaterial& mat = leader->material ? *leader->material : MaterialLibrary::Get().Default();

	instancedShader->UseShader();
	renderer.BindWorldCameraBlock();
	unsigned int id = instancedShader->GetID();
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));

	// Same material + lighting as Scene3DModel::DrawGeometry (groups are non-water).
	ApplyMaterial(id, mat, nullptr);
	ApplyLighting(id, renderer);
	leader->texture->UseTexture();

	// Motion vectors: the camera's motion only (grouped props stand still;
	// scene3d_instanced.vert passes no per-instance offset).
	const unsigned int buffers = Scene3DInternal::WorldDrawBuffers(id, false);
	if (buffers != 1u)
		Device().SetBoundDrawBufferMask(buffers);

	// Upload the instance matrices to the leader's mesh(es), draw all instances in
	// one call, then reset the instance count so later passes (shadow/outline that
	// call RenderMesh(0) on this same mesh) still draw non-instanced.
	DrawModelMeshes(id, leader->model3D.meshList, false, &mats);
	if (buffers != 1u)
		Device().SetBoundDrawBufferMask(1u);
	renderer.drawCallsPerFrame++;
#endif
}

std::vector<std::string> Scene3D::PointLightNames() const
{
	std::vector<std::string> names;
	for (const ScenePointLight& p : pointLights)
		names.push_back(p.name);
	return names;
}

std::vector<ScenePointLight>& Scene3D::GetPointLights()
{
	return pointLights;
}

std::vector<SceneSpotLight>& Scene3D::GetSpotLights()
{
	return spotLights;
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
	unsigned int id = shader->GetID();
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "model")), m);
	// How far the character moved since last frame, for motion vectors (0
	// unless Render enabled them; the body and head share the offset).
	const glm::vec3 motion = MotionWritesActive() ? Scene3DInternal::MotionOffset(this, position) : glm::vec3(0.0f);
	Device().SetUniform((int)(ShaderProgram::DrawUniformLocation(id, "motionOffset")), motion);
	renderer.BindWorldCameraBlock();   // view/projection: Camera block
	Device().SetUniform((int)(Device().UniformLocation(ProgramHandle(id), "theTexture")), (int)(0));

	Scene3D::Get().ApplyLighting(id, renderer);

	tex->UseTexture();
	quad->RenderMesh(0);
	renderer.drawCallsPerFrame++;
}

bool Scene3D::CharVisible(const Character3D* ch) const
{
	if (ch == nullptr) return false;
	if (!soloCharacter.empty())
		return ch->charName == soloCharacter;   // focused moment: only the opponent
	return renderCharacters;
}

void Scene3D::SpotlightCharacter(Game& game, const std::string& charName, bool on)
{
	focusSpotOn = false;
	if (!on)
		return;
	Character3D* ch = FindCharacter(charName);
	if (ch == nullptr)
		return;

	// up = -Y. position is the feet; light the torso from above + toward the camera.
	const float h = ch->worldHeight;
	glm::vec3 chest = ch->position + glm::vec3(0.0f, -0.6f * h, 0.0f);
	glm::vec3 toCam(game.renderer.camera.position.x - chest.x, 0.0f,
		game.renderer.camera.position.z - chest.z);
	if (glm::length(toCam) < 1e-3f) toCam = glm::vec3(0, 0, 1);
	toCam = glm::normalize(toCam);
	glm::vec3 spotPos = chest + glm::vec3(0.0f, -1.6f * h, 0.0f) + toCam * (0.7f * h);

	focusSpot.pos = spotPos;
	focusSpot.dir = glm::normalize(chest - spotPos);
	focusSpot.color = glm::vec3(1.0f, 0.96f, 0.86f);   // warm white
	focusSpot.range = 4.0f * h;
	focusSpot.intensity = 3.4f;
	focusSpot.innerDeg = 15.0f;
	focusSpot.outerDeg = 30.0f;
	focusSpot.on = true;
	focusSpotOn = true;
}

void Character3D::Render(const Renderer& renderer)
{
	if (renderer.camera.useOrthoCamera)
		return;

	// Scene used as a cutscene backdrop with its 3D cast suppressed (2D VN sprites
	// act instead), or a solo-focused moment (only the opponent) - skip if hidden.
	if (!Scene3D::Get().CharVisible(this))
		return;

	// When the game keeps characters out of the toon outline, also render to the
	// framebuffer's "is-character" mask (draw buffer 1) so the outline post-process
	// can skip these pixels. billboard3d.frag writes oMask=1.0. Characters still
	// write depth normally, so occlusion + depth sorting are unaffected.
	// Under temporal anti-aliasing, characters also write motion vectors
	// (draw buffer 2), so a walking character doesn't ghost.
	Scene3D& scene = Scene3D::Get();
	bool maskChar = scene.celShading && scene.outlineEnabled && !scene.outlineCharacters;
	const unsigned int buffers = (shader != nullptr)
		? Scene3DInternal::WorldDrawBuffers(shader->GetID(), maskChar) : (maskChar ? 3u : 1u);
	if (buffers != 1u)
		Device().SetBoundDrawBufferMask(buffers);

	// Body (or combined sprite) first, then head layered on top. Both are
	// full-canvas overlays that align by transparency; the head is pulled
	// slightly toward the camera so it isn't z-culled by the coplanar body.
	DrawQuad(renderer, bodyTex, 0.0f);
	if (headTex != nullptr)
		DrawQuad(renderer, headTex, 1.5f);

	if (buffers != 1u)
		Device().SetBoundDrawBufferMask(1u);
}

// --------------------------------------------------------------- manager

Scene3D& Scene3D::Get()
{
	static Scene3D instance;
	return instance;
}

// --- the lines of a .scene file that aren't data -----------------------------
// Comments, blank lines and lines the engine doesn't recognise are kept from
// the file a scene was loaded from, so a save writes them back: each block of
// them stays above the data line it was above, and a comment at the end of a
// data line stays at its end. Comments whose line is gone (a deleted model,
// the sun switched off...) move to the end of the file rather than vanish.
namespace
{
	struct SceneText
	{
		std::vector<std::string> header;     // above the first data line
		std::vector<std::string> trailer;    // below the last
		std::map<std::string, std::vector<std::string>> above;   // by line key
		std::map<std::string, std::string> after;                // "   # ..." ending a line, by key
		std::vector<std::string> keyOrder;   // keys in file order
		std::unordered_set<std::string> keySeen;
		std::vector<std::string> orphans;    // from models / characters deleted since loading
	};
	SceneText sceneText;

	bool IsBlankLine(const std::string& s)
	{
		return s.find_first_not_of(" \t\r") == std::string::npos;
	}

	void TrimBlankEnds(std::vector<std::string>& v)
	{
		while (!v.empty() && IsBlankLine(v.back()))
			v.pop_back();
		size_t first = 0;
		while (first < v.size() && IsBlankLine(v[first]))
			first++;
		v.erase(v.begin(), v.begin() + first);
	}

	// What a data line made: a model or character (by its object), a light,
	// camera or slot (by name), or a one-per-scene setting (by its tag).
	std::string SceneLineKey(const char* kind, const void* object)
	{
		std::ostringstream ss;
		ss << kind << object;
		return ss.str();
	}

	// The lines read since the last data line belong to the one just read
	// (above the first one, the part up to the last blank line is the header).
	void AttachSceneLines(const std::string& key, std::vector<std::string>& pending, const std::string& note, bool sawData)
	{
		if (!sawData)
		{
			size_t split = 0;
			for (size_t i = 0; i < pending.size(); i++)
				if (IsBlankLine(pending[i]))
					split = i + 1;
			sceneText.header.assign(pending.begin(), pending.begin() + split);
			TrimBlankEnds(sceneText.header);
			pending.erase(pending.begin(), pending.begin() + split);
		}
		TrimBlankEnds(pending);
		if (!pending.empty())
		{
			std::vector<std::string>& block = sceneText.above[key];
			block.insert(block.end(), pending.begin(), pending.end());
		}
		if (!note.empty())
			sceneText.after[key] = note;
		if (sceneText.keySeen.insert(key).second)
			sceneText.keyOrder.push_back(key);
	}

	void FinishSceneLines(std::vector<std::string>& pending, bool sawData)
	{
		TrimBlankEnds(pending);
		(sawData ? sceneText.trailer : sceneText.header) = pending;
	}

	// A model or character is being deleted: its lines go to the end of the
	// file (its object's address may be reused by the next one made).
	void OrphanSceneLines(const std::string& key)
	{
		auto a = sceneText.above.find(key);
		if (a != sceneText.above.end())
		{
			for (const std::string& line : a->second)
				if (!IsBlankLine(line))
					sceneText.orphans.push_back(line);
			sceneText.above.erase(a);
		}
		auto n = sceneText.after.find(key);
		if (n != sceneText.after.end())
		{
			const size_t hash = n->second.find('#');
			sceneText.orphans.push_back(hash == std::string::npos ? n->second : n->second.substr(hash));
			sceneText.after.erase(n);
		}
	}

	// The scene file: the data lines (key, text; an empty text = a section
	// break) with the kept lines put back around them.
	void WriteSceneLines(std::ostream& out, const std::vector<std::pair<std::string, std::string>>& lines)
	{
		std::vector<std::string> text;
		auto blank = [&]()
		{
			if (!text.empty() && !IsBlankLine(text.back()))
				text.push_back(std::string());
		};
		if (sceneText.header.empty())
		{
			text.push_back("# Saved by the in-game 3D editor.");
			text.push_back("# model <obj> <tex> <x> <y> <z> <yaw> <scale> [solid] [tag <VALUE>]");
			text.push_back("#   optional: rot <pitch> <roll>   scaleaxis <sx> <sy> <sz>");
		}
		else
		{
			text = sceneText.header;
		}
		blank();

		std::unordered_set<std::string> written;
		for (const std::pair<std::string, std::string>& l : lines)
		{
			if (l.second.empty())
			{
				blank();
				continue;
			}
			std::string line = l.second;
			if (!l.first.empty() && written.insert(l.first).second)
			{
				auto a = sceneText.above.find(l.first);
				if (a != sceneText.above.end() && !a->second.empty())
				{
					blank();
					text.insert(text.end(), a->second.begin(), a->second.end());
				}
				auto n = sceneText.after.find(l.first);
				if (n != sceneText.after.end())
					line += n->second;
			}
			text.push_back(line);
		}

		// Comments whose line is gone.
		std::vector<std::string> lost;
		for (const std::string& key : sceneText.keyOrder)
		{
			if (written.count(key) != 0)
				continue;
			auto a = sceneText.above.find(key);
			if (a != sceneText.above.end())
				for (const std::string& line : a->second)
					if (!IsBlankLine(line))
						lost.push_back(line);
			auto n = sceneText.after.find(key);
			if (n != sceneText.after.end())
			{
				const size_t hash = n->second.find('#');
				lost.push_back(hash == std::string::npos ? n->second : n->second.substr(hash));
			}
		}
		lost.insert(lost.end(), sceneText.orphans.begin(), sceneText.orphans.end());
		if (!lost.empty())
		{
			blank();
			text.insert(text.end(), lost.begin(), lost.end());
		}
		if (!sceneText.trailer.empty())
		{
			blank();
			text.insert(text.end(), sceneText.trailer.begin(), sceneText.trailer.end());
		}
		while (!text.empty() && IsBlankLine(text.back()))
			text.pop_back();
		for (const std::string& t : text)
			out << t << "\n";
	}
}

void Scene3DInternal::RenameSceneLine(const std::string& kind, const std::string& from, const std::string& to)
{
	const std::string a = kind + ":" + from, b = kind + ":" + to;
	if (a == b)
		return;
	auto moveKey = [&](auto& map)
	{
		auto it = map.find(a);
		if (it != map.end())
		{
			map[b] = it->second;
			map.erase(a);
		}
	};
	moveKey(sceneText.above);
	moveKey(sceneText.after);
	for (std::string& k : sceneText.keyOrder)
		if (k == a)
			k = b;
	sceneText.keySeen.erase(a);
	sceneText.keySeen.insert(b);
}

bool Scene3D::RenameCamera(const std::string& from, const std::string& to)
{
	if (to.empty() || from == to || cameras.find(from) == cameras.end() || cameras.find(to) != cameras.end())
		return false;
	cameras[to] = cameras[from];
	cameras.erase(from);
	for (std::string& name : cameraOrder)
		if (name == from)
			name = to;
	Scene3DInternal::RenameSceneLine("camera", from, to);
	return true;
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
		modelTweens.clear();
		characters.clear();
		solids.clear();
	grounds.clear();
		cameras.clear();
		cameraOrder.clear();
		anchors.clear();
	}

	// A fresh scene always shows its cast (a prior cutscene may have hidden it).
	renderCharacters = true;
	soloCharacter = "";
	focusSpotOn = false;

	// Dedicated shaders (all the VN shaders are 2D): lit for scene models,
	// unlit alpha-cutout for character billboards
	if (shader == nullptr)
	{
		shader = new ShaderProgram(-1, modelShaderVert.c_str(), modelShaderFrag.c_str());
	}
	if (billboardShader == nullptr)
	{
		billboardShader = new ShaderProgram(-1, billboardShaderVert.c_str(), billboardShaderFrag.c_str());
	}
	if (instancedShader == nullptr)
	{
		// Instanced variant of the model shader (dup opaque props). Shares the
		// model fragment shader; reads the per-instance model matrix from attribs.
		instancedShader = new ShaderProgram(-1, instancedShaderVert.c_str(), modelShaderFrag.c_str());
	}
	EdgeShader();
	if (shadowDepthShader == nullptr)
	{
		shadowDepthShader = new ShaderProgram(-1, "data/shaders/shadow_depth.vert", "data/shaders/shadow_depth.frag");
	}
	if (pointShadowShader == nullptr)
	{
		pointShadowShader = new ShaderProgram(-1, "data/shaders/point_shadow_depth.vert", "data/shaders/point_shadow_depth.frag");
	}
	if (weatherShader == nullptr)
	{
		weatherShader = new ShaderProgram(-1, weatherShaderVert.c_str(), weatherShaderFrag.c_str());
	}

	// (Re)load the material library so "mat <name>" tokens can resolve.
	// KINJO_MATERIALS=<file> loads that file instead, to try materials out
	// (e.g. in a scratch test scene) without touching the game's.
	const char* materialsOverride = std::getenv("KINJO_MATERIALS");
	if (materialsOverride != nullptr && materialsOverride[0] != '\0')
		MaterialLibrary::Get().Load(game, materialsOverride);
	else
		MaterialLibrary::Get().Load(game);

	// Reset lighting to defaults; the file's light directives override it
	ambientColor = glm::vec3(0.08f, 0.08f, 0.10f);
	dirLight = SceneDirLight();
	pointLights.clear();
	spotLights.clear();
	shadowCasterLight.clear();   // reset the caster override per scene
	pointShadowEverRendered = false;   // force the point-shadow cubes to re-render
	skyTexPath.clear();   // a scene without a "sky" line has none
	weatherType = WeatherType::None;   // a scene without a "weather" line has none
	weatherIntensity = 1.0f;
	weatherInit = false;               // reseed the volume for the new scene
	stormSeeded = false;               // rearm lightning for the new scene
	flashIntensity = 0.0f;
	reflashTimer = thunderTimer = -1.0f;
	hasFountain = false;               // a scene without a "fountain" line has none
	fountainInit = false;
	season = Season::Summer;           // a scene without a "season" line = summer
	Scene3DInternal::RestoreGameToonSettings();   // and the game's cel / outline settings
	SetSceneExposure(0.0f);            // a scene without an "exposure" line = the project default
	SetSceneBloom(-1.0f);              // likewise "bloom"
	SetSceneIBL(-1.0f, -1.0f);         // and "ibl"
	Scene3DInternal::SetSceneShadowDistance(-1.0f);   // and "shadowdistance"
	SetSceneAO(-1.0f, -1.0f);          // and "ao"
	SetSceneColorGrade("", 1.0f, 0.0f);   // and "grade"
	::SetDepthOfField(500.0f, 0.0f, 0.0f);   // and "dof"
	SetSceneFog(false, FogSettings(), 0.0f);   // and "fog"
	SetSceneDistanceFog(false, DistanceFogSettings(), 0.0f);   // and "distfog"

	// Shared unit billboard quad: x[-0.5,0.5], y[0,1] (base at origin), z=0,
	// with dummy normals so Mesh::CreateMesh's stride-8 layout is satisfied
	if (billboardQuad == nullptr)
	{
		float qv[] = {
			// pos                uv          normal
			-0.5f, 0.0f, 0.0f,   0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
			 0.5f, 0.0f, 0.0f,   1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
			 0.5f, 1.0f, 0.0f,   1.0f, 0.0f,  0.0f, 0.0f, 1.0f,
			-0.5f, 1.0f, 0.0f,   0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
		};
		unsigned int qi[] = { 0, 1, 2, 0, 2, 3 };
		billboardQuad = new Mesh();
		billboardQuad->CreateMesh(qv, qi, 32, 6, 8, 3, 5);
	}

	sceneText = SceneText();
	std::vector<std::string> pending;   // comment / blank / unrecognised lines since the last data line
	bool sawData = false;
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		const size_t first = line.find_first_not_of(" \t");
		if (first == std::string::npos || line[first] == '#')
		{
			pending.push_back(line);   // kept for saving
			continue;
		}
		// A comment ending a data line: set apart, and put back on save.
		std::string data = line, note;
		for (size_t i = first + 1; i < line.size(); i++)
		{
			if (line[i] == '#' && (line[i - 1] == ' ' || line[i - 1] == '\t'))
			{
				size_t j = i;
				while (j > 0 && (line[j - 1] == ' ' || line[j - 1] == '\t'))
					j--;
				data = line.substr(0, j);
				note = line.substr(j);
				break;
			}
		}

		std::istringstream ss(data);
		std::string tag;
		ss >> tag;
		const size_t modelsBefore = models.size();
		const size_t charactersBefore = characters.size();

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
			m->texture = SceneColorTexture(game, texPath);
			m->objPath = objPath;
			m->texPath = texPath;

			std::string flag;
			while (ss >> flag)
			{
				if (flag == "solid")
					m->solid = true;
				else if (flag == "walk")
					m->walkable = true;
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
				else if (flag == "if")             // if <guard...>  - availability guard
				{                                  //   (rest of line; may contain spaces)
					std::string g;
					std::getline(ss, g);
					size_t a = g.find_first_not_of(" \t");
					m->guard = (a == std::string::npos) ? "" : g.substr(a);
				}
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
				if (ComputeSolidBox(objPath, pos, yaw, scale, m->scaleAxis, box))
					solids.push_back(box);
			}
			// Walkable top: characters stand on this surface (GetGroundHeight)
			if (m->walkable)
			{
				SolidBox box;
				if (ComputeSolidBox(objPath, pos, yaw, scale, m->scaleAxis, box))
					grounds.push_back({ box.minX, box.maxX, box.minZ, box.maxZ, box.minY });
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
		else if (tag == "slot")
		{
			// slot <name> <x> <y> <z> [yaw]  - a named stand-point where the
			// character schedule can place a character (yaw optional, default 0).
			SceneAnchor a;
			ss >> a.name >> a.position.x >> a.position.y >> a.position.z;
			if (!(ss >> a.yaw))
				a.yaw = 0.0f;
			anchors.push_back(a);
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
			// point <name> <x> <y> <z> <r> <g> <b> <range> <intensity> [off] [flash <hz> [phase]] [if <guard>]
			ScenePointLight p;
			ss >> p.name >> p.pos.x >> p.pos.y >> p.pos.z
				>> p.color.r >> p.color.g >> p.color.b
				>> p.range >> p.intensity;
			p.flashPeak = p.intensity;
			// optional trailing modifiers, in order: "flash <hz> <phase>" then
			// "if <guard...>" (a strobe and/or an availability guard).
			std::string kw;
			while (ss >> kw)
			{
				if (kw == "off")
					p.on = false;   // starts switched off (a script turns it on)
				else if (kw == "flash")
					ss >> p.flashHz >> p.flashPhase;
				else if (kw == "if")
				{
					std::string g;
					std::getline(ss, g);
					size_t a = g.find_first_not_of(" \t");
					p.guard = (a == std::string::npos) ? "" : g.substr(a);
					break;   // guard consumes the rest of the line
				}
			}
			pointLights.push_back(p);
		}
		else if (tag == "spot")
		{
			// spot <name> <x> <y> <z> <dx> <dy> <dz> <r> <g> <b>
			//      <range> <intensity> <innerDeg> <outerDeg> [off]
			SceneSpotLight s;
			ss >> s.name >> s.pos.x >> s.pos.y >> s.pos.z
				>> s.dir.x >> s.dir.y >> s.dir.z
				>> s.color.r >> s.color.g >> s.color.b
				>> s.range >> s.intensity >> s.innerDeg >> s.outerDeg;
			std::string kw;
			while (ss >> kw)
				if (kw == "off")
					s.on = false;   // [off]: starts switched off
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

			if (mode == "layered")
			{
				std::string folder, bodyPose, headExpr;
				glm::vec3 pos;
				float height = 220.0f;
				ss >> folder >> bodyPose >> headExpr
					>> pos.x >> pos.y >> pos.z >> height;
				AddCharacter(game, charName, folder, bodyPose, headExpr, pos, height);
			}
			else if (mode == "combined")
			{
				std::string spritePath, bodyPath;
				glm::vec3 pos;
				float height = 220.0f;
				ss >> spritePath >> pos.x >> pos.y >> pos.z >> height;

				Character3D* ch = new Character3D(pos);
				ch->worldHeight = height;
				ch->bodyTex = SceneColorTexture(game, spritePath);
				ch->headTex = nullptr;  // combined sprites need no layering
				bodyPath = spritePath;
				ch->mode = "combined";
				ch->spritePath = spritePath;
				ch->charName = charName;
				ComputeFigureBounds(ch, bodyPath, std::string());
				ch->shader = billboardShader;
				ch->quad = billboardQuad;
				if (ch->bodyTex == nullptr)
					std::cout << "Scene3D: character has no body/sprite texture" << std::endl;
				characters.push_back(ch);
				game.entities.push_back(ch);
			}
		}
		else if (tag == "weather")
		{
			// weather <rain|snow|storm> [intensity 0..1]  - a camera-following
			// particle volume of falling rain streaks or drifting snow. "storm"
			// is heavy rain plus dynamic lightning flashes and delayed thunder.
			std::string kind;
			float intensity = 1.0f;
			ss >> kind;
			if (ss >> intensity) {}   // optional
			if (kind == "rain")       weatherType = WeatherType::Rain;
			else if (kind == "snow")  weatherType = WeatherType::Snow;
			else if (kind == "storm") weatherType = WeatherType::Storm;
			else                      weatherType = WeatherType::None;
			weatherIntensity = glm::clamp(intensity, 0.0f, 1.0f);
			weatherInit = false;
			stormSeeded = false;
			flashIntensity = 0.0f;
			reflashTimer = thunderTimer = -1.0f;
		}
		else if (tag == "fountain")
		{
			// fountain <x> <y> <z> [jetSpeed] [fallDist] [spread] [dropSize] [count] [stretch]
			// - a point emitter that sprays droplets up from (x,y,z) and lets them
			// arc back down. The trailing params are optional (editor-tunable).
			glm::vec3 p(0.0f);
			float jet = 480.0f, fall = 175.0f, spread = 60.0f, dropSize = 9.0f, stretch = 3.5f;
			int count = 340;
			ss >> p.x >> p.y >> p.z;
			if (ss >> jet) {}
			if (ss >> fall) {}
			if (ss >> spread) {}
			if (ss >> dropSize) {}
			if (ss >> count) {}
			if (ss >> stretch) {}
			SetFountain(p, jet, fall, spread);
			SetFountainDropSize(dropSize);
			SetFountainCount(count);
			SetFountainStretch(stretch);
		}
		else if (tag == "season")
		{
			// season <spring|summer|autumn|winter> - swaps seasonal-material
			// textures. Stored now; applied after all models finish loading.
			std::string kind;
			ss >> kind;
			season = (kind == "spring") ? Season::Spring
				: (kind == "autumn") ? Season::Autumn
				: (kind == "winter") ? Season::Winter : Season::Summer;
		}
		else if (tag == "exposure")
		{
			// exposure <multiplier> - this scene's exposure in a linear-workflow
			// project (render/ColorPipeline.h); ignored otherwise.
			float e = 0.0f;
			if (ss >> e)
				SetSceneExposure(e);
		}
		else if (tag == "bloom")
		{
			// bloom <strength 0..1> - this scene's bloom in a linear-workflow
			// project (0 = none); ignored otherwise.
			float b = -1.0f;
			if (ss >> b)
				SetSceneBloom(b);
		}
		else if (tag == "ibl")
		{
			// ibl <diffuse> [specular] - image-based lighting from this scene's
			// sky (linear workflow; 0 = off). One value sets both.
			float d = -1.0f, s = -1.0f;
			if (ss >> d)
			{
				if (!(ss >> s))
					s = d;
				SetSceneIBL(d, s);
			}
		}
		else if (tag == "shadowdistance")
		{
			// shadowdistance <world units> - how far this scene's cascaded sun
			// shadows reach (renderer.dat shadowDistance otherwise).
			float d = -1.0f;
			if (ss >> d)
				Scene3DInternal::SetSceneShadowDistance(d);
		}
		else if (tag == "fog")
		{
			// fog <density> [height falloff] [r g b] [anisotropy] [noise] - this
			// scene's volumetric fog in a linear-workflow project
			// (render/VolumetricFog.h; density 0 = none, even in rain).
			FogSettings fog;
			if (ss >> fog.density)
			{
				float v = 0.0f;
				if (ss >> v) fog.heightFalloff = v;
				glm::vec3 c;
				if (ss >> c.r >> c.g >> c.b) fog.color = c;
				if (ss >> v) fog.anisotropy = v;
				if (ss >> v) fog.noise = v;
				SetSceneFog(true, fog, 0.0f);
			}
		}
		else if (tag == "distfog")
		{
			// distfog <r g b> <near> <far> - this scene's distance fog, any colour
			// mode (render/DistanceFog.h): none nearer than near, all of it from
			// far on. `distfog off` turns renderer.dat's default off here.
			std::string first;
			if (ss >> first)
			{
				DistanceFogSettings fog;
				if (first == "off")
					SetSceneDistanceFog(true, fog, 0.0f);   // on = false
				else
				{
					std::istringstream rest(first);
					if ((rest >> fog.color.r) && (ss >> fog.color.g >> fog.color.b >> fog.nearDistance >> fog.farDistance))
					{
						fog.on = true;
						SetSceneDistanceFog(true, fog, 0.0f);
					}
					else
						std::cout << "Scene3D: distfog wants <r g b> <near> <far> or off" << std::endl;
				}
			}
		}
		else if (tag == "dof")
		{
			// dof <focus distance> <aperture> - this scene's depth of field in a
			// linear-workflow project (aperture = blur in 720p pixels at
			// infinity; render/DepthOfField.h).
			float focus = 0.0f, aperture = 0.0f;
			if (ss >> focus >> aperture)
				::SetDepthOfField(focus, aperture, 0.0f);
		}
		else if (tag == "grade")
		{
			// grade <lut.png|none> [strength] - this scene's colour grade in a
			// linear-workflow project (render/ColorGrading.h).
			std::string lutPath;
			float strength = 1.0f;
			if (ss >> lutPath)
			{
				if (!(ss >> strength))
					strength = 1.0f;
				SetSceneColorGrade(lutPath, strength, 0.0f);
			}
		}
		else if (tag == "cel" || tag == "outline" || tag == "outlinechars")
		{
			// cel on|off, outline on|off, outlinechars on|off - this scene's own
			// cel shading / outline (the game's values otherwise).
			std::string v;
			if (ss >> v)
			{
				const bool on = !(v == "off" || v == "0" || v == "false");
				using TS = Scene3DInternal::ToonSetting;
				const TS which = (tag == "cel") ? TS::CelShading : (tag == "outline") ? TS::Outline : TS::OutlineCharacters;
				Scene3DInternal::OwnToonSetting(which);
				(which == TS::CelShading ? celShading : which == TS::Outline ? outlineEnabled : outlineCharacters) = on;
			}
		}
		else if (tag == "outlinewidth" || tag == "outlinedepth")
		{
			// outlinewidth <pixels>, outlinedepth <edge threshold>
			float v = 0.0f;
			if (ss >> v)
			{
				using TS = Scene3DInternal::ToonSetting;
				Scene3DInternal::OwnToonSetting(tag == "outlinewidth" ? TS::OutlineWidth : TS::OutlineDepth);
				(tag == "outlinewidth" ? outlineWidth : outlineDepthThreshold) = v;
			}
		}
		else if (tag == "outlinecolor")
		{
			// outlinecolor <r> <g> <b>
			glm::vec3 c;
			if (ss >> c.r >> c.g >> c.b)
			{
				Scene3DInternal::OwnToonSetting(Scene3DInternal::ToonSetting::OutlineColor);
				outlineColor = c;
			}
		}
		else if (tag == "ao")
		{
			// ao <strength 0..1> [radius] - this scene's ambient occlusion in a
			// linear-workflow project (0 = none; radius in world units).
			float strength = -1.0f, radius = -1.0f;
			if (ss >> strength)
			{
				ss >> radius;
				SetSceneAO(strength, radius);
			}
		}
		else
		{
			// Not a tag this engine knows (a newer engine's, or a typo): kept
			// as it is, so saving doesn't lose it.
			pending.push_back(line);
			continue;
		}

		// What the line made, so its comments stay with it.
		std::string key;
		{
			std::istringstream names(data);
			std::string skip, name;
			names >> skip >> name;
			if (tag == "model")
				key = (models.size() > modelsBefore) ? SceneLineKey("model:", models.back()) : std::string();
			else if (tag == "character")
				key = (characters.size() > charactersBefore) ? SceneLineKey("character:", characters.back()) : std::string();
			else if (tag == "point" || tag == "spot" || tag == "camera" || tag == "slot")
				key = tag + ":" + name;
			else
				key = "tag:" + tag;
		}
		if (key.empty())
		{
			pending.push_back(line);   // it made nothing (couldn't be read): kept as it is
			continue;
		}
		AttachSceneLines(key, pending, note, sawData);
		pending.clear();
		sawData = true;
	}
	FinishSceneLines(pending, sawData);

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

	// A global weather override (debug/CLI flag or story-wide storm) wins over the
	// scene's authored weather.
	if (forcedWeather != WeatherType::None)
	{
		weatherType = forcedWeather;
		weatherIntensity = forcedWeatherIntensity;
		weatherInit = false;
	}

	// Apply the authored season now that all models (and their materials) are
	// loaded, swapping seasonal-material textures to the season's variant.
	if (season != Season::Summer)
		SetSeason(game, season);

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
		return SceneColorTexture(game, flat);
	}

	// Animation-frame folder: use the first frame as the static pose
	std::string framed = base + "/1.png";
	std::ifstream probe2(framed);
	if (probe2.good())
	{
		probe2.close();
		if (resolvedPath) *resolvedPath = framed;
		return SceneColorTexture(game, framed);
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

// --- solid collision ------------------------------------------------------

bool Scene3D::ReadObjLocalAABB(const std::string& objPath,
	glm::vec3& lo, glm::vec3& hi) const
{
	// An OBJ's local bounds never change while the game runs, but this used to
	// re-open and re-parse the whole file on EVERY call. That made
	// RebuildSolids() cost one file parse per solid model, and RemoveModel()
	// calls RebuildSolids() unconditionally - so deleting a single model in a
	// scene with ~100 solids meant ~100 file parses. Games that remove models
	// repeatedly at runtime (a game vacuuming up props, SceneLogic's
	// "keep N of tag" thinning loop, the editor's tile tools) paid that over
	// and over; one such case dropped the frame rate from 60 to ~15.
	//
	// Cache the parsed bounds per path. Also speeds up scene LOADING, where
	// duplicate props previously re-parsed the same OBJ once per instance.
	// Keyed on the path string; OBJ files are not edited at runtime, so the
	// entries never go stale.
	static std::map<std::string, std::pair<glm::vec3, glm::vec3>> s_aabbCache;
	{
		auto cached = s_aabbCache.find(objPath);
		if (cached != s_aabbCache.end())
		{
			lo = cached->second.first;
			hi = cached->second.second;
			// A miss was cached as an inverted box; report it as a failure
			// again rather than re-reading a file we know we cannot open.
			return lo.x <= hi.x;
		}
	}

	// Remember the outcome (success or failure) before returning.
	struct CacheOnExit
	{
		std::map<std::string, std::pair<glm::vec3, glm::vec3>>* cache;
		const std::string* path;
		glm::vec3* lo;
		glm::vec3* hi;
		~CacheOnExit() { (*cache)[*path] = { *lo, *hi }; }
	} cacheOnExit{ &s_aabbCache, &objPath, &lo, &hi };

	// Sentinel for "could not read": an inverted box.
	lo = glm::vec3(1e9f);
	hi = glm::vec3(-1e9f);

	// The model loader has usually just loaded this very file and knows its
	// bounds from the vertex data. Re-parsing a large OBJ's text here cost as
	// much as loading it (0.5 s per character frame in Debug).
#ifdef USE_ASSIMP
	{
		float mlo[3], mhi[3];
		if (Model::LoadedBounds(objPath, mlo, mhi))
		{
			lo = glm::vec3(mlo[0], mlo[1], mlo[2]);
			hi = glm::vec3(mhi[0], mhi[1], mhi[2]);
			return true;
		}
	}
#endif

	std::ifstream file(objPath);
	if (!file.is_open())
	{
		std::cout << "Scene3D: cannot open OBJ " << objPath << std::endl;
		return false;
	}

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
		if (m->guardHidden) continue;   // hidden by its availability guard: not pickable
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
	// The data lines in order, each with the key that finds the comments kept
	// from loading (WriteSceneLines puts them back); an empty text = a break.
	std::vector<std::pair<std::string, std::string>> lines;
	auto add = [&](const std::string& key, const std::ostringstream& text) { lines.emplace_back(key, text.str()); };
	auto section = [&]() { lines.emplace_back(std::string(), std::string()); };

	for (Scene3DModel* m : models)
	{
		if (IsRuntimeModel(m))
			continue;   // a game's own geometry (AddRuntimeModel), rebuilt by the game
		std::ostringstream o;
		glm::vec3 p = m->position;
		o << "model " << m->objPath << " " << m->texPath << " "
			<< p.x << " " << p.y << " " << p.z << " "
			<< m->yawDeg << " " << m->modelScale;
		if (m->solid)
			o << " solid";
		if (m->walkable)
			o << " walk";
		if (!m->interactionTag.empty())
			o << " tag " << m->interactionTag;
		if (m->pitchDeg != 0.0f || m->rollDeg != 0.0f)
			o << " rot " << m->pitchDeg << " " << m->rollDeg;
		if (m->scaleAxis.x != 1.0f || m->scaleAxis.y != 1.0f || m->scaleAxis.z != 1.0f)
			o << " scaleaxis " << m->scaleAxis.x << " " << m->scaleAxis.y << " " << m->scaleAxis.z;
		if (!m->materialName.empty())
			o << " mat " << m->materialName;
		if (m->IsWater())
			o << " water " << m->water.amplitude << " " << m->water.waveScale << " "
				<< m->water.shoreFade << " " << m->water.choppy << " " << m->water.specular
				<< " " << m->water.shininess << " " << m->water.opacity;
		// "if <guard>" must be LAST (parse reads the rest of the line into the guard).
		if (!m->guard.empty())
			o << " if " << m->guard;
		add(SceneLineKey("model:", m), o);
	}
	section();

	for (Character3D* ch : characters)
	{
		std::ostringstream o;
		glm::vec3 p = ch->position;
		if (ch->mode == "combined")
		{
			o << "character " << ch->charName << " combined "
				<< ch->spritePath << " "
				<< p.x << " " << p.y << " " << p.z << " " << ch->worldHeight;
		}
		else
		{
			o << "character " << ch->charName << " layered "
				<< ch->folder << " " << ch->bodyPose << " " << ch->headExpr << " "
				<< p.x << " " << p.y << " " << p.z << " " << ch->worldHeight;
		}
		add(SceneLineKey("character:", ch), o);
	}
	section();

	// Sky + lighting
	if (!skyTexPath.empty())
	{
		std::ostringstream o;
		o << "sky " << skyTexPath << " " << skyRadiusVal;
		add("tag:sky", o);
	}
	{
		std::ostringstream o;
		o << "ambient " << ambientColor.r << " " << ambientColor.g << " " << ambientColor.b;
		add("tag:ambient", o);
	}
	if (dirLight.diffuse > 0.0f)
	{
		std::ostringstream o;
		o << "light " << dirLight.dir.x << " " << dirLight.dir.y << " "
			<< dirLight.dir.z << " " << dirLight.color.r << " "
			<< dirLight.color.g << " " << dirLight.color.b << " "
			<< dirLight.diffuse;
		add("tag:light", o);
	}
	for (const ScenePointLight& p : pointLights)
	{
		// A strobing light is saved at its authored peak, not mid-blink.
		std::ostringstream o;
		const float intensity = (p.flashHz > 0.0f) ? p.flashPeak : p.intensity;
		o << "point " << p.name << " " << p.pos.x << " " << p.pos.y << " "
			<< p.pos.z << " " << p.color.r << " " << p.color.g << " "
			<< p.color.b << " " << p.range << " " << intensity;
		if (!p.on)
			o << " off";
		if (p.flashHz > 0.0f)
			o << " flash " << p.flashHz << " " << p.flashPhase;
		if (!p.guard.empty())
			o << " if " << p.guard;   // runs to the end of the line
		add("point:" + p.name, o);
	}
	for (const SceneSpotLight& sp : spotLights)
	{
		std::ostringstream o;
		o << "spot " << sp.name << " " << sp.pos.x << " " << sp.pos.y << " "
			<< sp.pos.z << " " << sp.dir.x << " " << sp.dir.y << " " << sp.dir.z
			<< " " << sp.color.r << " " << sp.color.g << " " << sp.color.b
			<< " " << sp.range << " " << sp.intensity << " " << sp.innerDeg
			<< " " << sp.outerDeg;
		if (!sp.on)
			o << " off";
		add("spot:" + sp.name, o);
	}
	// One-per-scene settings, each only when the scene sets it.
	auto setting = [&](const char* tag, const std::function<void(std::ostringstream&)>& write)
	{
		std::ostringstream o;
		o << tag << " ";
		write(o);
		add(std::string("tag:") + tag, o);
	};
	// Which point light casts shadows (empty = auto-pick the strongest).
	if (!shadowCasterLight.empty())
		setting("shadowlight", [&](std::ostringstream& o) { o << shadowCasterLight; });
	// Weather (rain / snow / storm), if authored on this scene.
	if (weatherType != WeatherType::None)
		setting("weather", [&](std::ostringstream& o)
		{
			o << (weatherType == WeatherType::Rain ? "rain" : weatherType == WeatherType::Snow ? "snow" : "storm")
				<< " " << weatherIntensity;
		});
	// Fountain spray, if authored on this scene.
	if (hasFountain)
		setting("fountain", [&](std::ostringstream& o)
		{
			o << fountainPos.x << " " << fountainPos.y << " " << fountainPos.z
				<< " " << fountainJetSpeed << " " << fountainFallDist << " " << fountainSpread
				<< " " << fountainDropSize << " " << fountainCount << " " << fountainStretch;
		});
	// Season (foliage texture set), if not the default summer.
	if (season != Season::Summer)
		setting("season", [&](std::ostringstream& o)
		{
			o << (season == Season::Spring ? "spring" : season == Season::Autumn ? "autumn" : "winter");
		});
	// Exposure (linear workflow), if this scene sets its own.
	if (SceneExposure() > 0.0f)
		setting("exposure", [&](std::ostringstream& o) { o << SceneExposure(); });
	if (SceneBloom() >= 0.0f)
		setting("bloom", [&](std::ostringstream& o) { o << SceneBloom(); });
	{
		float iblDiffuse = -1.0f, iblSpecular = -1.0f;
		GetSceneIBL(iblDiffuse, iblSpecular);
		if (iblDiffuse >= 0.0f)
			setting("ibl", [&](std::ostringstream& o)
			{
				o << iblDiffuse << " " << (iblSpecular >= 0.0f ? iblSpecular : iblDiffuse);
			});
	}
	if (Scene3DInternal::SceneShadowDistance() > 0.0f)
		setting("shadowdistance", [&](std::ostringstream& o) { o << Scene3DInternal::SceneShadowDistance(); });
	{
		std::string gradePath;
		float gradeStrength = 1.0f;
		GetSceneColorGrade(gradePath, gradeStrength);
		if (!gradePath.empty())
			setting("grade", [&](std::ostringstream& o) { o << gradePath << " " << gradeStrength; });
		FogSettings fog;
		if (GetSceneFog(fog))
			setting("fog", [&](std::ostringstream& o)
			{
				o << fog.density << " " << fog.heightFalloff << " " << fog.color.r << " " << fog.color.g
					<< " " << fog.color.b << " " << fog.anisotropy << " " << fog.noise;
			});
		DistanceFogSettings distFog;
		if (GetSceneDistanceFog(distFog))
			setting("distfog", [&](std::ostringstream& o)
			{
				if (!distFog.on)
					o << "off";
				else
					o << distFog.color.r << " " << distFog.color.g << " " << distFog.color.b
						<< " " << distFog.nearDistance << " " << distFog.farDistance;
			});
		float dofFocus = 0.0f, dofAperture = 0.0f;
		GetSceneDepthOfField(dofFocus, dofAperture);
		if (dofAperture > 0.0f && DepthOfFieldTarget().empty())
			setting("dof", [&](std::ostringstream& o) { o << dofFocus << " " << dofAperture; });
	}
	{
		float aoStrength = -1.0f, aoRadius = -1.0f;
		GetSceneAO(aoStrength, aoRadius);
		if (aoStrength >= 0.0f || aoRadius > 0.0f)
		{
			// A radius alone still needs a strength first: -1 = the project's.
			setting("ao", [&](std::ostringstream& o)
			{
				o << aoStrength;
				if (aoRadius > 0.0f)
					o << " " << aoRadius;
			});
		}
	}
	{
		// The cel / outline settings this scene sets itself.
		using TS = Scene3DInternal::ToonSetting;
		if (Scene3DInternal::SceneOwnsToonSetting(TS::CelShading))
			setting("cel", [&](std::ostringstream& o) { o << (celShading ? "on" : "off"); });
		if (Scene3DInternal::SceneOwnsToonSetting(TS::Outline))
			setting("outline", [&](std::ostringstream& o) { o << (outlineEnabled ? "on" : "off"); });
		if (Scene3DInternal::SceneOwnsToonSetting(TS::OutlineCharacters))
			setting("outlinechars", [&](std::ostringstream& o) { o << (outlineCharacters ? "on" : "off"); });
		if (Scene3DInternal::SceneOwnsToonSetting(TS::OutlineWidth))
			setting("outlinewidth", [&](std::ostringstream& o) { o << outlineWidth; });
		if (Scene3DInternal::SceneOwnsToonSetting(TS::OutlineDepth))
			setting("outlinedepth", [&](std::ostringstream& o) { o << outlineDepthThreshold; });
		if (Scene3DInternal::SceneOwnsToonSetting(TS::OutlineColor))
			setting("outlinecolor", [&](std::ostringstream& o)
			{
				o << outlineColor.r << " " << outlineColor.g << " " << outlineColor.b;
			});
	}
	section();

	// Cameras (preserve load order; the first is the default view)
	for (const std::string& name : cameraOrder)
	{
		auto it = cameras.find(name);
		if (it == cameras.end())
			continue;
		std::ostringstream o;
		const CamPose& c = it->second;
		o << "camera " << name << " " << c.position.x << " " << c.position.y
			<< " " << c.position.z << " " << c.pitch << " " << c.yaw;
		add("camera:" + name, o);
	}

	// Named anchors (schedule stand-points). yaw omitted when 0.
	for (const SceneAnchor& a : anchors)
	{
		std::ostringstream o;
		o << "slot " << a.name << " " << a.position.x << " " << a.position.y << " " << a.position.z;
		if (a.yaw != 0.0f)
			o << " " << a.yaw;
		add("slot:" + a.name, o);
	}

	WriteSceneLines(out, lines);
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
	grounds.clear();
	for (Scene3DModel* m : models)
	{
		if (!m->solid && !m->walkable)
			continue;
		SolidBox box;
		if (!ComputeSolidBox(m->objPath, m->position, m->yawDeg, m->modelScale, m->scaleAxis, box))
			continue;
		if (m->solid)
			solids.push_back(box);
		if (m->walkable)
			grounds.push_back({ box.minX, box.maxX, box.minZ, box.maxZ, box.minY });
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
			const std::string ext = e.path().extension().generic_string();
			const bool gltfFile = (ext == ".gltf" || ext == ".glb");
			if (!e.is_regular_file() || (ext != ".obj" && !gltfFile))
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
				if (gltfFile)
				{
					def.tex = "-";   // its own materials
					def.solid = false;
				}
				else if (dt != kDefaults.end())
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
	m->texture = SceneColorTexture(game, def.tex);
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

namespace
{
	// Runtime models (AddRuntimeModel) are told apart by their source path.
	const char* const kRuntimePrefix = "runtime:";
}

bool Scene3D::IsRuntimeModel(const Scene3DModel* m)
{
	return m != nullptr && m->objPath.compare(0, 8, kRuntimePrefix) == 0;
}

Scene3DModel* Scene3D::AddRuntimeModel(Game& game, Mesh* mesh, const std::string& texture,
	const std::string& material, const glm::vec3& localMin, const glm::vec3& localMax, const glm::vec3& pos)
{
	if (shader == nullptr || mesh == nullptr)   // no scene loaded
		return nullptr;
	Scene3DModel* m = new Scene3DModel(pos);
	m->shader = shader;
	m->texture = SceneColorTexture(game, texture);
	m->texPath = texture;
	// The mesh's address keys it: two models share instanced draws only when
	// they share the mesh (the grouping is by path + texture + material).
	std::ostringstream key;
	key << kRuntimePrefix << (const void*)mesh;
	m->objPath = key.str();
	m->model3D.meshList.push_back(mesh);   // borrowed: Model never frees its meshes
	m->loaded = true;
	m->materialName = material;
	m->material = material.empty() ? nullptr : MaterialLibrary::Get().Find(material);
	if (!material.empty() && m->material == nullptr)
		std::cout << "Scene3D: runtime model material '" << material << "' not found" << std::endl;
	m->localMin = localMin;
	m->localMax = localMax;
	m->hasLocalBounds = true;
	RecomputeModelBounds(m);
	models.push_back(m);
	game.entities.push_back(m);
	return m;
}

void Scene3D::RemoveRuntimeModels(Game& game)
{
	bool solidGone = false;
	for (size_t i = 0; i < models.size();)
	{
		Scene3DModel* m = models[i];
		if (!IsRuntimeModel(m))
		{
			i++;
			continue;
		}
		modelTweens.erase(std::remove_if(modelTweens.begin(), modelTweens.end(),
			[m](const ModelTween& t) { return t.model == m; }), modelTweens.end());
		solidGone = solidGone || m->solid;
		models.erase(models.begin() + i);
		game.ShouldDeleteEntity(m);
	}
	if (solidGone)
		RebuildSolids();
}

Skybox* Scene3D::GetSkybox() const
{
	return skybox;
}

bool Scene3D::RemoveModel(Game& game, int index)
{
	if (index < 0 || index >= (int)models.size())
		return false;
	Scene3DModel* m = models[index];
	OrphanSceneLines(SceneLineKey("model:", m));
	modelTweens.erase(std::remove_if(modelTweens.begin(), modelTweens.end(),
		[m](const ModelTween& t) { return t.model == m; }), modelTweens.end());
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
	OrphanSceneLines(SceneLineKey("character:", c));
	characters.erase(characters.begin() + index);
	game.ShouldDeleteEntity(c);
	std::cout << "Scene3D: removed character " << index << std::endl;
	return true;
}

Character3D* Scene3D::AddCharacter(Game& game, const std::string& name, const std::string& folder,
	const std::string& bodyPose, const std::string& headExpr, const glm::vec3& pos, float height)
{
	std::string bodyPath, headPath;
	Character3D* ch = new Character3D(pos);
	ch->worldHeight = height;
	ch->bodyTex = ResolveTexture(game, folder, "body", bodyPose, &bodyPath);
	ch->headTex = ResolveTexture(game, folder, "head", headExpr, &headPath);
	ch->mode = "layered";
	ch->folder = folder;
	ch->bodyPose = bodyPose;
	ch->headExpr = headExpr;
	ch->charName = name;
	ComputeFigureBounds(ch, bodyPath, headPath);
	ch->shader = billboardShader;
	ch->quad = billboardQuad;
	if (ch->bodyTex == nullptr)
		std::cout << "Scene3D: character " << name << " has no body texture (folder " << folder << ")" << std::endl;
	characters.push_back(ch);
	game.entities.push_back(ch);
	return ch;
}

bool Scene3D::RemoveCharacter(Game& game, Character3D* ch)
{
	for (size_t i = 0; i < characters.size(); i++)
		if (characters[i] == ch)
			return RemoveCharacter(game, (int)i);
	return false;
}

bool Scene3D::GetAnchor(const std::string& name, glm::vec3& outPos, float& outYaw) const
{
	for (const SceneAnchor& a : anchors)
	{
		if (a.name == name)
		{
			outPos = a.position;
			outYaw = a.yaw;
			return true;
		}
	}
	return false;
}

bool Scene3D::RemoveAnchorAt(int index)
{
	if (index < 0 || index >= (int)anchors.size())
		return false;
	anchors.erase(anchors.begin() + index);
	return true;
}

bool Scene3D::ComputeSolidBox(const std::string& objPath, const glm::vec3& pos,
	float yawDeg, float scale, const glm::vec3& scaleAxis, SolidBox& out) const
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
	m = glm::scale(m, glm::vec3(scale) * scaleAxis);

	out.minX = out.minZ = out.minY = 1e9f;
	out.maxX = out.maxZ = out.maxY = -1e9f;
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
		out.minY = glm::min(out.minY, w.y);
		out.maxY = glm::max(out.maxY, w.y);
	}
	return true;
}

void Scene3D::ResolveAgainstSolids(glm::vec3& pos, float radius) const
{
	// Historical behavior: solids are infinite vertical columns (bodyHeight
	// sentinel < 0 disables the Y overlap test).
	ResolveAgainstSolids(pos, radius, -1.0f, 0.0f);
}

void Scene3D::ResolveAgainstSolids(glm::vec3& pos, float radius,
	float bodyHeight, float maxStepUp) const
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
			// Y-aware mode: skip boxes that don't overlap the body span
			// (feet at pos.y, head at pos.y - bodyHeight; up = -Y), and
			// boxes whose top is within step range (climbable, handled by
			// GetGroundHeight rather than a wall push).
			if (bodyHeight >= 0.0f)
			{
				bool risesAboveStep = b.minY < pos.y - maxStepUp;
				bool belowHead = b.maxY > pos.y - bodyHeight;
				if (!risesAboveStep || !belowHead)
					continue;
			}

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

bool Scene3D::GetGroundHeight(const glm::vec3& pos, float maxStepUp, float& outY) const
{
	// Up = -Y: a surface is "no more than maxStepUp above the feet" when its
	// topY >= pos.y - maxStepUp; anything below the feet is a fall target.
	// Among the eligible surfaces under this XZ point, the smallest y (the
	// highest surface) wins.
	bool found = false;
	float best = 1e9f;
	for (const GroundBox& g : grounds)
	{
		if (pos.x < g.minX || pos.x > g.maxX || pos.z < g.minZ || pos.z > g.maxZ)
			continue;
		if (g.topY < pos.y - maxStepUp)
			continue;   // too high to step onto
		if (g.topY < best)
		{
			best = g.topY;
			found = true;
		}
	}
	if (found)
		outY = best;
	return found;
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

int Scene3D::AnimateModelTurn(const std::string& tag, float yawDeg, float seconds)
{
	int matched = 0;
	for (Scene3DModel* m : models)
	{
		if (m == nullptr || tag.empty() || m->interactionTag != tag)
			continue;
		matched++;
		modelTweens.erase(std::remove_if(modelTweens.begin(), modelTweens.end(),
			[m](const ModelTween& t) { return t.model == m && t.turn; }), modelTweens.end());
		if (seconds <= 0.0f)
		{
			m->yawDeg = yawDeg;
			RecomputeModelBounds(m);
			continue;
		}
		ModelTween t;
		t.model = m;
		t.turn = true;
		t.from = glm::vec3(m->yawDeg, 0.0f, 0.0f);
		t.to = glm::vec3(yawDeg, 0.0f, 0.0f);
		t.seconds = seconds;
		modelTweens.push_back(t);
	}
	if (matched == 0)
		std::cout << "Scene3D: no model tagged '" << tag << "' to turn" << std::endl;
	return matched;
}

int Scene3D::AnimateModelMove(const std::string& tag, const glm::vec3& pos, float seconds)
{
	int matched = 0;
	bool solidMoved = false;
	for (Scene3DModel* m : models)
	{
		if (m == nullptr || tag.empty() || m->interactionTag != tag)
			continue;
		matched++;
		modelTweens.erase(std::remove_if(modelTweens.begin(), modelTweens.end(),
			[m](const ModelTween& t) { return t.model == m && !t.turn; }), modelTweens.end());
		if (seconds <= 0.0f)
		{
			m->position = pos;
			RecomputeModelBounds(m);
			solidMoved = solidMoved || m->solid;
			continue;
		}
		ModelTween t;
		t.model = m;
		t.from = m->position;
		t.to = pos;
		t.seconds = seconds;
		modelTweens.push_back(t);
	}
	if (solidMoved)
		RebuildSolids();
	if (matched == 0)
		std::cout << "Scene3D: no model tagged '" << tag << "' to move" << std::endl;
	return matched;
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

void Scene3D::SetSkyRadius(float radius)
{
	skyRadiusVal = std::max(radius, 100.0f);
	if (skybox != nullptr)
		skybox->skyRadius = skyRadiusVal;
}

void Scene3D::SetAuthoredSky(Game& game, const std::string& path)
{
	if (path.empty())
	{
		if (skybox != nullptr)
		{
			game.ShouldDeleteEntity(skybox);
			skybox = nullptr;
		}
		skyTexPath.clear();
		return;
	}
	if (skybox == nullptr)
	{
		if (skyRadiusVal <= 0.0f)
			skyRadiusVal = 4000.0f;
		skybox = new Skybox(game, path, skyRadiusVal);
		game.entities.push_back(skybox);
	}
	else
	{
		SetSkyTexture(game, path);
	}
	skyTexPath = path;
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
	modelTweens.clear();
	characters.clear();
	solids.clear();
	grounds.clear();
	cameras.clear();
	cameraOrder.clear();
	active = false;
	gliding = false;
	currentScene.clear();
	Scene3DInternal::RestoreGameToonSettings();
	sceneText = SceneText();
	SetSceneExposure(0.0f);   // back to the project defaults
	SetSceneBloom(-1.0f);
	SetSceneIBL(-1.0f, -1.0f);
	Scene3DInternal::SetSceneShadowDistance(-1.0f);
	SetSceneAO(-1.0f, -1.0f);
	SetSceneColorGrade("", 1.0f, 0.0f);
	::SetDepthOfField(500.0f, 0.0f, 0.0f);
	SetSceneFog(false, FogSettings(), 0.0f);
	SetSceneDistanceFog(false, DistanceFogSettings(), 0.0f);
	Scene3DInternal::ForgetMotion();

	RestoreOrtho(game);
	std::cout << "Scene3D: unloaded, back to 2D" << std::endl;
}

void Scene3D::SetExposure(float multiplier, float fadeSeconds)
{
	SetSceneExposure(multiplier, fadeSeconds);
}

float Scene3D::GetExposure() const
{
	return SceneExposure();
}

void Scene3D::SetBloom(float strength)
{
	SetSceneBloom(strength);
}

float Scene3D::GetBloom() const
{
	return SceneBloom();
}

void Scene3D::SetIBL(float diffuse, float specular)
{
	SetSceneIBL(diffuse, specular);
}

void Scene3D::SetAmbientOcclusion(float strength, float radius)
{
	SetSceneAO(strength, radius);
}

void Scene3D::SetColorGrade(const std::string& lutPath, float strength, float fadeSeconds)
{
	SetSceneColorGrade(lutPath, strength, fadeSeconds);
}

void Scene3D::SetDepthOfField(float focusDistance, float aperture, float fadeSeconds)
{
	::SetDepthOfField(focusDistance, aperture, fadeSeconds);
}

void Scene3D::SetDepthOfFieldTarget(const std::string& characterName, float aperture, float fadeSeconds)
{
	::SetDepthOfFieldTarget(characterName, aperture, fadeSeconds);
}

void Scene3D::UpdateDepthOfField(const Renderer& renderer)
{
	const std::string& name = DepthOfFieldTarget();
	if (name.empty() || !active)
		return;
	Character3D* ch = FindCharacter(name);
	if (ch == nullptr)
		return;
	// Focus on the face: most of the way up the figure (world up is -Y).
	const glm::vec3 head = ch->position + glm::vec3(0.0f, -1.0f, 0.0f) * (ch->worldHeight * 0.85f);
	const glm::vec4 v = renderer.camera.CalculateViewMatrix() * glm::vec4(head, 1.0f);
	SetDepthOfFieldFocus(-v.z);
}

void Scene3D::SetFog(float density, float fadeSeconds)
{
	SetSceneFogDensity(density, fadeSeconds);
}

void Scene3D::SetDistanceFog(bool on, const glm::vec3& color, float nearDistance, float farDistance, float fadeSeconds)
{
	DistanceFogSettings fog;
	fog.on = on;
	if (on)
	{
		fog.color = color;
		fog.nearDistance = nearDistance;
		fog.farDistance = farDistance;
	}
	SetSceneDistanceFog(true, fog, fadeSeconds);
}

bool Scene3D::WantsVolumetricFog(const Renderer& renderer)
{
	// Weather brings its own haze when the scene sets no fog of its own.
	float weatherFog = 0.0f;
	if (weatherType == WeatherType::Rain)
		weatherFog = 0.00030f;
	else if (weatherType == WeatherType::Snow)
		weatherFog = 0.00035f;
	else if (weatherType == WeatherType::Storm)
		weatherFog = 0.00055f;
	SetWeatherFogDensity(weatherFog * weatherIntensity);
	return active && !renderer.camera.useOrthoCamera && FogActive();
}

void Scene3D::RenderVolumetricFog(Game& game, const Renderer& renderer)
{
	const int width = ViewTargetWidth(game), height = ViewTargetHeight(game);
	const unsigned int program = BeginFogMarch(width, height);
	if (program == 0)
		return;
	const Camera& cam = renderer.camera;
	const TextureHandle depth(ViewTargetFrameBuffer(game)->depthTexture);
	Device().UseProgram(ProgramHandle(program));
	renderer.BindWorldCameraBlock();
	ApplyLighting(program, renderer);   // sun, lights, shadows, sky: what the fog scatters
	DrawFogMarch(glm::inverse(cam.projection * cam.CalculateViewMatrix()), depth, width, height,
		renderer.now * 0.001f);
	Device().SetViewport(0, 0, width, height);
}

void Scene3D::NotifyCameraCut()
{
	ResetTemporalHistory();
}

bool Scene3D::GetSkySource(Texture*& sky, Texture*& next, float& blend, glm::vec3& tint) const
{
	if (!active || skybox == nullptr || skybox->GetSprite() == nullptr || skybox->GetSprite()->texture == nullptr)
		return false;
	const Sprite* s = skybox->GetSprite();
	sky = s->texture;
	next = skybox->nextTexture;
	blend = (next != nullptr) ? std::min(std::max(skybox->blendToNext, 0.0f), 1.0f) : 0.0f;
	tint = glm::vec3(s->color.r, s->color.g, s->color.b) / 255.0f;
	return true;
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

bool Scene3D::FocusBounds(Game& game, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
	float seconds, float fillFrac)
{
	glm::vec3 size = aabbMax - aabbMin;
	if (size.x < 0.01f && size.y < 0.01f && size.z < 0.01f)
		return false;   // degenerate / bounds not computed

	glm::vec3 center = (aabbMin + aabbMax) * 0.5f;

	// Fit both the vertical extent (in the vertical FOV) and the horizontal
	// extent (in the wider horizontal FOV, scaled by aspect); take whichever
	// needs the greater distance so the whole box stays in frame.
	float vHalf = glm::radians(perspFovDeg * 0.5f);
	float tanV = tanf(vHalf);
	float aspect = (game.screenHeight > 0)
		? (float)game.screenWidth / (float)game.screenHeight : 1.777f;
	float vExtent = size.y;
	float hExtent = sqrtf(size.x * size.x + size.z * size.z);  // loose horizontal diagonal
	float distV = (vExtent * 0.5f) / tanV;
	float distH = (hExtent * 0.5f) / (aspect * tanV);
	float dist = std::max(distV, distH);
	if (fillFrac > 0.05f) dist /= fillFrac;   // pull back so the box fills ~fillFrac
	if (dist < 1.0f) dist = 1.0f;

	// Keep the horizontal side the camera is currently on (dolly toward the box).
	glm::vec3 camPos = game.renderer.camera.position;
	glm::vec3 sideXZ(camPos.x - center.x, 0.0f, camPos.z - center.z);
	if (glm::length(sideXZ) < 0.001f)
		sideXZ = glm::vec3(0, 0, 1);
	sideXZ = glm::normalize(sideXZ);

	// Level, eye-line shot at the box center - or, for a flat object (a ticket,
	// a toilet seat, a key), from 45 degrees above, so its top is what's seen
	// rather than its edge.
	CamPose pose;
	if (vExtent < 0.35f * hExtent)
	{
		const float k = 0.7071f;
		pose.position = center + sideXZ * (dist * k);
		pose.position.y = center.y - dist * k;   // world up is -Y
	}
	else
	{
		pose.position = center + sideXZ * dist;
		pose.position.y = center.y;
	}

	glm::vec3 front = glm::normalize(pose.position - center);
	pose.pitch = glm::degrees(asinf(glm::clamp(front.y, -1.0f, 1.0f)));
	pose.yaw = glm::degrees(atan2f(front.z, front.x));

	GlideToPose(pose, seconds);
	focusCharName = "";   // not a character glide: skip the character centering verifier
	std::cout << "Scene3D: focus bounds over " << glideSeconds << "s, dist " << dist
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

	// Model animations (scene3d model <tag> turn|move): eased to the target.
	bool solidMoved = false;
	for (size_t i = 0; i < modelTweens.size();)
	{
		ModelTween& t = modelTweens[i];
		t.elapsed += dtSec;
		const float k = std::min(t.elapsed / t.seconds, 1.0f);
		const glm::vec3 v = t.from + (t.to - t.from) * (k * k * (3.0f - 2.0f * k));
		if (t.turn)
			t.model->yawDeg = v.x;
		else
			t.model->position = v;
		RecomputeModelBounds(t.model);
		if (k >= 1.0f)
		{
			solidMoved = solidMoved || t.model->solid;
			modelTweens.erase(modelTweens.begin() + i);
		}
		else
			i++;
	}
	if (solidMoved)
		RebuildSolids();

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

	// Strobe any flashing point lights (police/emergency). A square wave overrides
	// the intensity: full for the first half of the cycle, near-off for the second.
	// Two lights at opposite phase (0 and 0.5) alternate (e.g. red then blue).
	flashClock += dtSec;
	for (ScenePointLight& p : pointLights)
	{
		if (p.flashHz <= 0.0f || p.guardHidden) continue;
		float ph = flashClock * p.flashHz + p.flashPhase;
		ph -= std::floor(ph);                      // fract -> 0..1
		p.intensity = p.flashPeak * (ph < 0.5f ? 1.0f : 0.04f);
		p.on = true;
	}

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

	// Advance the weather particle volume (keeps it centered on the camera).
	if (weatherType != WeatherType::None)
		UpdateWeather(game.renderer.camera.position, dtSec);

	// Storm: drive the lightning strike timing, flash decay, and delayed thunder.
	if (weatherType == WeatherType::Storm)
		UpdateLightning(game, dtSec);

	// Fountain spray (ballistic droplets).
	if (hasFountain)
		UpdateFountain(dtSec);
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
	ResetTemporalHistory();   // a cut: last frame's image doesn't belong to this view

	std::cout << "Scene3D: camera '" << camName << "' pos ("
		<< cam.position.x << "," << cam.position.y << "," << cam.position.z
		<< ") pitch " << cam.pitch << " yaw " << cam.yaw << std::endl;

	// Wrong-facing-camera guard: the view matrix looks along MINUS front
	// (Camera::CalculateViewMatrix), which trips up scene authors deriving
	// poses from the front-vector formula - yaw 90 faces -Z (270 faces +Z),
	// and pitch must be NEGATIVE to look down (visual down = +Y). A camera
	// aimed away from every model renders a silent blank screen that has cost
	// hours to diagnose; one dot product per model on camera jumps makes it a
	// loud console warning instead.
	if (!models.empty())
	{
		const float yawR = glm::radians((float)cam.yaw);
		const float pitchR = glm::radians((float)cam.pitch);
		const glm::vec3 lookDir(-cosf(yawR) * cosf(pitchR),
			-sinf(pitchR),
			-sinf(yawR) * cosf(pitchR));

		bool anyInFront = false;
		for (Scene3DModel* m : models)
		{
			if (m == nullptr || m->guardHidden)
				continue;
			if (glm::dot(m->position - cam.position, lookDir) > 0.0f)
			{
				anyInFront = true;
				break;
			}
		}

		if (!anyInFront)
		{
			std::cout << "Scene3D: WARNING - camera '" << camName << "' faces AWAY from all "
				<< models.size() << " models; nothing will render (blank screen). "
				<< "The engine looks along MINUS front: yaw 90 faces -Z, yaw 270 faces +Z, "
				<< "and pitch must be NEGATIVE to look down (visual down = +Y). "
				<< "Check this camera's pitch/yaw signs and which side of the content it sits on."
				<< std::endl;
		}
	}

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

// --- a scene's own cel-shading / outline settings (Scene3DInternal.h) -------

namespace
{
	const int kToonCount = (int)Scene3DInternal::ToonSetting::Count;
	bool toonOwned[kToonCount] = {};
	bool toonSaved = false;   // the game's values below are held
	bool gameCel = false, gameOutline = true, gameOutlineChars = true;
	float gameOutlineWidth = 3.0f, gameOutlineDepth = 4.0f;
	glm::vec3 gameOutlineColor(0.0f);
}

void Scene3DInternal::OwnToonSetting(ToonSetting s)
{
	Scene3D& scene = Scene3D::Get();
	if (!toonSaved)
	{
		gameCel = scene.celShading;
		gameOutline = scene.outlineEnabled;
		gameOutlineChars = scene.outlineCharacters;
		gameOutlineWidth = scene.outlineWidth;
		gameOutlineDepth = scene.outlineDepthThreshold;
		gameOutlineColor = scene.outlineColor;
		toonSaved = true;
	}
	toonOwned[(int)s] = true;
}

bool Scene3DInternal::SceneOwnsToonSetting(ToonSetting s)
{
	return toonOwned[(int)s];
}

void Scene3DInternal::ResetToonSetting(ToonSetting s)
{
	if (!toonOwned[(int)s])
		return;
	toonOwned[(int)s] = false;
	Scene3D& scene = Scene3D::Get();
	switch (s)
	{
	case ToonSetting::CelShading: scene.celShading = gameCel; break;
	case ToonSetting::Outline: scene.outlineEnabled = gameOutline; break;
	case ToonSetting::OutlineCharacters: scene.outlineCharacters = gameOutlineChars; break;
	case ToonSetting::OutlineWidth: scene.outlineWidth = gameOutlineWidth; break;
	case ToonSetting::OutlineDepth: scene.outlineDepthThreshold = gameOutlineDepth; break;
	case ToonSetting::OutlineColor: scene.outlineColor = gameOutlineColor; break;
	default: break;
	}
	bool any = false;
	for (bool owned : toonOwned)
		any = any || owned;
	if (!any)
		toonSaved = false;   // nothing held: the game may change its values freely again
}

void Scene3DInternal::RestoreGameToonSettings()
{
	for (int i = 0; i < kToonCount; i++)
		ResetToonSetting((ToonSetting)i);
}
