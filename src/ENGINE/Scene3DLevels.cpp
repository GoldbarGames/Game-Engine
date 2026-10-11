// Levels of detail for scene models (Scene3D::SetModelLevels, since
// 2026-10-10; Golf Galaxy's E5, docs/VISUAL_EFFECTS.md).
//
// A model's levels are meshes loaded like any model's (Model.cpp's cache, so
// the GPU-driven path's mesh pool has them too), kept here per model so
// Scene3DModel's layout is unchanged. Models with the same levels share one
// set. What a level is chosen by: the model's screen size, its bounding
// sphere's diameter over the screen's height, from the camera of the view
// being drawn (SetLevelCamera).
//
//   GPU-driven (Scene3DGpuDriven.cpp): each level is an instance of its own,
//     with the size range it draws in; cull_instances.comp keeps it when the
//     size is in range, and across a switch draws both levels with
//     complementary dithers (kLevelFade either side; shaders/scene3d.frag).
//   CPU paths: LevelMeshes picks the one level, no fade.

#include "Scene3D.h"
#include "Scene3DInternal.h"
#include "Camera.h"
#include "Model.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace
{
	// Sets by their files and sizes, shared by the models that have them.
	std::map<std::string, std::unique_ptr<Scene3DInternal::ModelLevels>> sets;
	std::unordered_map<const Scene3DModel*, const Scene3DInternal::ModelLevels*> levelsOf;

	glm::vec3 eye(0.0f);
	float scale = 1.0f;
}

void Scene3D::SetModelLevels(Scene3DModel* model, const std::vector<std::string>& objs, const std::vector<float>& below)
{
	if (model == nullptr)
		return;
	if (objs.empty() || objs.size() != below.size())
	{
		if (!objs.empty())
			std::cout << "Scene3D::SetModelLevels: " << objs.size() << " levels but " << below.size()
				<< " sizes; the model keeps its own mesh only" << std::endl;
		levelsOf.erase(model);
		return;
	}
	std::ostringstream key;
	for (size_t i = 0; i < objs.size(); i++)
		key << objs[i] << '\x01' << below[i] << '\x02';
	auto it = sets.find(key.str());
	if (it == sets.end())
	{
		auto set = std::make_unique<Scene3DInternal::ModelLevels>();
		for (size_t i = 0; i < objs.size(); i++)
		{
			std::vector<Mesh*> meshes;
#ifdef USE_ASSIMP
			if (!objs[i].empty())
			{
				Model level;
				level.LoadModel(objs[i]);   // shared through Model.cpp's cache: never freed
				meshes = level.meshList;
				if (meshes.empty())
					std::cout << "Scene3D::SetModelLevels: " << objs[i] << " didn't load; that level draws nothing" << std::endl;
			}
#endif
			set->meshes.push_back(meshes);
			set->below.push_back(below[i]);
		}
		it = sets.emplace(key.str(), std::move(set)).first;
	}
	levelsOf[model] = it->second.get();
}

float Scene3D::ModelScreenSize(const Scene3DModel* model, const Camera& camera) const
{
	if (model == nullptr)
		return 0.0f;
	const glm::vec4 sphere = Scene3DInternal::ModelSphere(*model, model->ModelMatrix());
	if (sphere.w < 0.0f)
		return 1e30f;
	// (the engine's projection flips y: its [1][1] is negative)
	return Scene3DInternal::SphereScreenSize(glm::vec3(sphere), sphere.w, camera.position, std::abs(camera.projection[1][1]));
}

const Scene3DInternal::ModelLevels* Scene3DInternal::LevelsOf(const Scene3DModel* model)
{
	if (levelsOf.empty())
		return nullptr;
	auto it = levelsOf.find(model);
	return it == levelsOf.end() ? nullptr : it->second;
}

void Scene3DInternal::ForgetModelLevels(const Scene3DModel* model)
{
	levelsOf.erase(model);
}

void Scene3DInternal::ClearModelLevels()
{
	levelsOf.clear();   // the sets stay: their meshes are the model cache's
}

void Scene3DInternal::SetLevelCamera(const glm::vec3& e, float s)
{
	eye = e;
	scale = std::abs(s);   // the engine's projection flips y: its [1][1] is negative
}

glm::vec3 Scene3DInternal::LevelEye()
{
	return eye;
}

float Scene3DInternal::LevelScale()
{
	return scale;
}

glm::vec4 Scene3DInternal::ModelSphere(const Scene3DModel& m, const glm::mat4& matrix)
{
	if (!m.hasLocalBounds)
		return glm::vec4(glm::vec3(matrix[3]), -1.0f);
	const glm::vec3 centre = (m.localMin + m.localMax) * 0.5f;
	const glm::vec3 half = (m.localMax - m.localMin) * 0.5f;
	const float axis = std::max(glm::length(glm::vec3(matrix[0])),
		std::max(glm::length(glm::vec3(matrix[1])), glm::length(glm::vec3(matrix[2]))));
	return glm::vec4(glm::vec3(matrix * glm::vec4(centre, 1.0f)), glm::length(half) * axis * 1.01f + 0.5f);
}

const std::vector<Mesh*>* Scene3DInternal::LevelMeshes(const Scene3DModel& model, const glm::mat4& matrix)
{
	const ModelLevels* levels = LevelsOf(&model);
	if (levels == nullptr)
		return &model.model3D.meshList;
	const glm::vec4 sphere = ModelSphere(model, matrix);
	if (sphere.w < 0.0f)
		return &model.model3D.meshList;
	const float size = SphereScreenSize(glm::vec3(sphere), sphere.w, eye, scale);
	size_t level = 0;
	while (level < levels->below.size() && size < levels->below[level])
		level++;
	if (level == 0)
		return &model.model3D.meshList;
	const std::vector<Mesh*>& meshes = levels->meshes[level - 1];
	return meshes.empty() ? nullptr : &meshes;
}
