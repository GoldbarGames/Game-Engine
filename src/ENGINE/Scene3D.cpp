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
		if (m->material != nullptr && m->material->IsTransparent() && !m->guardHidden)
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

	RenderState glass = CurrentRenderState();
	glass.depthWrite = false;   // don't write depth; keep depth TEST on
	ScopedRenderState scope(glass);
	for (Scene3DModel* m : transparent)
		m->DrawGeometry(renderer);
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
		Texture* t = game.spriteManager.GetImage(path, Texture::Filter::Smooth);
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
	if (!instancingEnabled)
		return;

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

	// Upload the instance matrices to the leader's mesh(es), draw all instances in
	// one call, then reset the instance count so later passes (shadow/outline that
	// call RenderMesh(0) on this same mesh) still draw non-instanced.
	for (Mesh* mesh : leader->model3D.meshList)
	{
		mesh->SetInstancesTransient(mats.data(), (unsigned int)mats.size());
		mesh->RenderMesh(0);
		mesh->ClearInstances();   // restore pristine VAO for the shadow/other passes
	}
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
	Scene3D& scene = Scene3D::Get();
	bool maskChar = scene.celShading && scene.outlineEnabled && !scene.outlineCharacters;
	if (maskChar)
		Device().SetBoundDrawBuffers(2);

	// Body (or combined sprite) first, then head layered on top. Both are
	// full-canvas overlays that align by transparency; the head is pulled
	// slightly toward the camera so it isn't z-culled by the coplanar body.
	DrawQuad(renderer, bodyTex, 0.0f);
	if (headTex != nullptr)
		DrawQuad(renderer, headTex, 1.5f);

	if (maskChar)
		Device().SetBoundDrawBuffers(1);
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
			// point <name> <x> <y> <z> <r> <g> <b> <range> <intensity> [flash <hz> [phase]]
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
				if (kw == "flash")
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
				ch->bodyTex = game.spriteManager.GetImage(spritePath, Texture::Filter::Smooth);
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
		if (m->walkable)
			out << " walk";
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
		// "if <guard>" must be LAST (parse reads the rest of the line into the guard).
		if (!m->guard.empty())
			out << " if " << m->guard;
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

	// Weather (rain / snow / storm), if authored on this scene.
	if (weatherType != WeatherType::None)
	{
		const char* kind = weatherType == WeatherType::Rain ? "rain"
			: weatherType == WeatherType::Snow ? "snow" : "storm";
		out << "weather " << kind << " " << weatherIntensity << "\n";
	}
	// Fountain spray, if authored on this scene.
	if (hasFountain)
		out << "fountain " << fountainPos.x << " " << fountainPos.y << " " << fountainPos.z
			<< " " << fountainJetSpeed << " " << fountainFallDist << " " << fountainSpread
			<< " " << fountainDropSize << " " << fountainCount << " " << fountainStretch << "\n";
	// Season (foliage texture set), if not the default summer.
	if (season != Season::Summer)
		out << "season " << (season == Season::Spring ? "spring"
			: season == Season::Autumn ? "autumn" : "winter") << "\n";
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

	// Named anchors (schedule stand-points). yaw omitted when 0.
	for (const SceneAnchor& a : anchors)
	{
		out << "slot " << a.name << " " << a.position.x << " " << a.position.y
			<< " " << a.position.z;
		if (a.yaw != 0.0f)
			out << " " << a.yaw;
		out << "\n";
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
	grounds.clear();
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

	// Level, eye-line shot at the box center.
	CamPose pose;
	pose.position = center + sideXZ * dist;
	pose.position.y = center.y;

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
