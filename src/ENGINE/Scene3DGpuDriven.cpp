// GPU-driven Scene3D models (Phase 1.5 item 11, docs/RENDERING_BACKEND_PLAN.md).
//
// Once per frame (UpdateCameraUBO) the opaque models become one instance list:
// a record per model (matrix, bounding sphere, flags) in a storage buffer,
// grouped into batches of models that share meshes, texture and material.
// Each batch's meshes come from the shared mesh pool (render/MeshPool.h), so
// any number of different meshes draw in one indirect multi-draw; commands are
// grouped by draw state (texture + material in the colour passes, the
// alpha-test texture in the depth passes), one multi-draw per group.
//
// Each view - the camera (AO prepass, world), a sun cascade, a point light's
// cube face - first runs cull_instances.comp: every instance's sphere against
// the view's frustum, the visible ones compacted in order into the view's
// region of a per-frame index list, and the counts written into the view's
// copy of the commands. The vertex shaders (*_gpu.vert) read each instance's
// index as a per-instance attribute, offset per command by baseInstance.
//
// Models the path can't take (water, see-through materials, a custom shader,
// meshes not in the pool, an old game shader copy without the Material block)
// keep drawing themselves exactly as before. Engine-internal; Scene3D's
// layout is unchanged (file statics, private methods only).

#include "Scene3D.h"
#include "Scene3DInternal.h"
#include "render/ColorPipeline.h"
#include "ModelMaterials.h"
#include "Renderer.h"
#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "ShaderSources.h"
#include "EnginePaths.h"
#include "RenderState.h"
#include "globals.h"
#include "render/RenderDevice.h"
#include "render/MeshPool.h"
#include "render/TemporalAA.h"
#include "render/HiZ.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using Scene3DInternal::ProgramHasBlock;

namespace
{
	// ---- settings -------------------------------------------------------------
	bool settingsLoaded = false;
	bool wanted = true;

	bool GpuDrivenWanted()
	{
		if (!settingsLoaded)
		{
			settingsLoaded = true;
			auto config = GetMapStringsFromFile(RendererConfigPath());
			wanted = !(config.count("gpuDriven") > 0 && config["gpuDriven"] == "0");
			if (const char* v = std::getenv("KINJO_GPU_DRIVEN"))
				wanted = (v[0] == '1');
		}
		return wanted && Device().SupportsGpuDriven();
	}

	// ---- this frame's list ------------------------------------------------------
	// std430 mirror of SceneInstance (shaders/scene_instances.glsl).
	struct InstanceRecord
	{
		glm::mat4 model;
		glm::vec4 sphere;   // xyz centre, w radius (< 0: unknown, never culled)
		glm::uvec4 info;    // x batch, y flags
	};
	static_assert(sizeof(InstanceRecord) == 96, "InstanceRecord must match shaders/scene_instances.glsl");

	const uint32_t kFlagColour = 1u;   // drawn in the world and the AO prepass
	const uint32_t kFlagCaster = 2u;   // casts sun and point-light shadows
	enum { kColourSet = 0, kDepthSet = 1 };

	struct Batch
	{
		const Scene3DModel* leader = nullptr;
		uint32_t firstInstance = 0, instanceCount = 0;
	};

	// One multi-draw: consecutive commands that share draw state.
	struct Group
	{
		Texture* texture = nullptr;                  // albedo (colour) or alpha-test texture (depth)
		const SceneMaterial* material = nullptr;     // colour: the model's scene material
		const ModelMaterial* own = nullptr;          // colour: a glTF mesh's own material
		uint32_t firstCommand = 0, commandCount = 0;
	};

	bool frameActive = false;
	std::vector<InstanceRecord> instances;
	std::vector<const Scene3DModel*> instanceModels;   // parallel to instances (motion vectors)
	std::vector<Batch> batches;
	std::vector<uint32_t> batchTable;                  // 6 words per batch (cull_instances.comp)
	std::vector<uint32_t> batchCommands;               // command indices, listed per batch and set
	std::vector<DrawIndexedIndirectCommand> templates[2];
	std::vector<Group> groups[2];
	std::unordered_set<const Scene3DModel*> colourModels, casterModels;
	// The rest, which the CPU loops still draw (so they needn't walk every model).
	std::vector<Scene3DModel*> cpuColourModels, cpuCasterModels;
	uint32_t worldDrawnFrame = 0, frameNumber = 0;

	// A model's transform as last computed, so still models cost a compare.
	struct CachedTransform
	{
		glm::vec3 position, scale, localMin, localMax;
		float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
		bool hasLocal = false;
		glm::mat4 model;
		glm::vec4 sphere;
	};
	std::unordered_map<const Scene3DModel*, CachedTransform> transforms;

	// ---- GPU buffers, one set per frame in flight ------------------------------
	// Rotated in step with the transient ring, whose end-of-frame fence wait
	// guarantees the GPU is done with a set before it is rewritten.
	const int kSlots = 3;
	struct Buffer
	{
		BufferHandle handle;
		size_t capacity = 0;
	};
	struct Slot
	{
		Buffer instances, motion, batches, lists, commands, visible;
	};
	Slot slots[kSlots];
	int slot = 0;
	int viewIndex = 0;   // views culled this frame (each has its own commands + visible region)

	// Grow-only: a bigger request replaces the buffer (draws already recorded
	// keep the old one until the GPU is done with it).
	BufferHandle Ensure(Buffer& b, size_t bytes)
	{
		if (bytes == 0)
			bytes = 16;
		if (b.capacity < bytes)
		{
			RenderDevice& device = Device();
			device.DestroyBuffer(b.handle);
			b.capacity = std::max(bytes, b.capacity * 2);
			b.handle = device.CreateBuffer(b.capacity, nullptr, BufferUsage::Dynamic);
		}
		return b.handle;
	}

	// ---- programs ----------------------------------------------------------------
	bool programsTried = false, programsOk = false;
	ShaderProgram* worldProgram = nullptr;
	ShaderProgram* prepassProgram = nullptr;
	ShaderProgram* shadowProgram = nullptr;
	ShaderProgram* pointProgram = nullptr;
	unsigned int cullProgram = 0;
	struct CullLocations { int planes[6], commandSet, commandBase, visibleBase, flags, batchCount; } cullLoc;

	bool EnsurePrograms()
	{
		if (programsTried)
			return programsOk;
		programsTried = true;
		worldProgram = new ShaderProgram(-1, "data/shaders/scene3d_gpu.vert", "data/shaders/scene3d.frag");
		prepassProgram = new ShaderProgram(-1, "data/shaders/ao_prepass_gpu.vert", "data/shaders/ao_prepass.frag");
		// The engine's depth fragment shaders, not a game's older copy: those
		// declare as loose uniforms what these vertex shaders' block holds.
		const std::string depthFrag = EngineShaderDir() + "shadow_depth.frag";
		const std::string pointFrag = EngineShaderDir() + "point_shadow_depth.frag";
		shadowProgram = new ShaderProgram(-1, "data/shaders/shadow_depth_gpu.vert", depthFrag.c_str());
		pointProgram = new ShaderProgram(-1, "data/shaders/point_shadow_depth_gpu.vert", pointFrag.c_str());
		cullProgram = CreateComputeProgramFromFile("data/shaders/cull_instances.comp");
		programsOk = worldProgram->GetID() != 0 && prepassProgram->GetID() != 0 && shadowProgram->GetID() != 0
			&& pointProgram->GetID() != 0 && cullProgram != 0;
		if (!programsOk)
		{
			std::cout << "GPU-driven models: a shader failed to build; models draw the CPU way" << std::endl;
			return false;
		}
		RenderDevice& device = Device();
		const ProgramHandle cull(cullProgram);
		for (int p = 0; p < 6; p++)
			cullLoc.planes[p] = device.UniformLocation(cull, ("draw.planes[" + std::to_string(p) + "]").c_str());
		cullLoc.commandSet = device.UniformLocation(cull, "draw.commandSet");
		cullLoc.commandBase = device.UniformLocation(cull, "draw.commandBase");
		cullLoc.visibleBase = device.UniformLocation(cull, "draw.visibleBase");
		cullLoc.flags = device.UniformLocation(cull, "draw.flags");
		cullLoc.batchCount = device.UniformLocation(cull, "draw.batchCount");
		std::cout << "GPU-driven models: on (GPU culling + indirect multi-draws)" << std::endl;
		return true;
	}

	// The six frustum planes of a view-projection (Gribb-Hartmann), normalized,
	// pointing inward: a point p is inside when dot(n, p) + d >= 0 for all six.
	void FrustumPlanes(const glm::mat4& m, glm::vec4 planes[6])
	{
		const glm::vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
		const glm::vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
		const glm::vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
		const glm::vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);
		planes[0] = row3 + row0;
		planes[1] = row3 - row0;
		planes[2] = row3 + row1;
		planes[3] = row3 - row1;
		planes[4] = row3 + row2;
		planes[5] = row3 - row2;
		for (int i = 0; i < 6; i++)
		{
			const float len = glm::length(glm::vec3(planes[i]));
			if (len > 0.0f)
				planes[i] /= len;
		}
	}

	// A model's matrix and world bounding sphere (from its local bounds, so a
	// model a game moves without refreshing its picking box is still culled right).
	const CachedTransform& TransformOf(const Scene3DModel* m)
	{
		CachedTransform& c = transforms[m];
		const glm::vec3 scale = m->EffectiveScale();
		const bool same = c.position == m->position && c.scale == scale && c.yaw == m->yawDeg
			&& c.pitch == m->pitchDeg && c.roll == m->rollDeg && c.hasLocal == m->hasLocalBounds
			&& c.localMin == m->localMin && c.localMax == m->localMax;
		if (same)
			return c;
		c.position = m->position;
		c.scale = scale;
		c.yaw = m->yawDeg;
		c.pitch = m->pitchDeg;
		c.roll = m->rollDeg;
		c.hasLocal = m->hasLocalBounds;
		c.localMin = m->localMin;
		c.localMax = m->localMax;
		c.model = m->ModelMatrix();
		if (m->hasLocalBounds)
		{
			const glm::vec3 centre = (m->localMin + m->localMax) * 0.5f;
			const glm::vec3 half = (m->localMax - m->localMin) * 0.5f;
			const float axis = std::max(glm::length(glm::vec3(c.model[0])),
				std::max(glm::length(glm::vec3(c.model[1])), glm::length(glm::vec3(c.model[2]))));
			c.sphere = glm::vec4(glm::vec3(c.model * glm::vec4(centre, 1.0f)), glm::length(half) * axis * 1.01f + 0.5f);
		}
		else
		{
			c.sphere = glm::vec4(glm::vec3(c.model[3]), -1.0f);
		}
		return c;
	}

	struct ColourKey
	{
		Texture* texture;
		const SceneMaterial* material;
		const ModelMaterial* own;
		bool operator==(const ColourKey& o) const
		{
			return texture == o.texture && material == o.material && own == o.own;
		}
	};
	struct ColourKeyHash
	{
		size_t operator()(const ColourKey& k) const
		{
			return std::hash<const void*>()(k.texture) ^ (std::hash<const void*>()(k.material) * 31u)
				^ (std::hash<const void*>()(k.own) * 131u);
		}
	};

	struct BatchKey
	{
		const Mesh* firstMesh;
		size_t meshCount;
		Texture* texture;
		const SceneMaterial* material;
		bool operator==(const BatchKey& o) const
		{
			return firstMesh == o.firstMesh && meshCount == o.meshCount && texture == o.texture
				&& material == o.material;
		}
	};
	struct BatchKeyHash
	{
		size_t operator()(const BatchKey& k) const
		{
			return std::hash<const void*>()(k.firstMesh) ^ (k.meshCount * 7919u)
				^ (std::hash<const void*>()(k.texture) * 31u) ^ (std::hash<const void*>()(k.material) * 131u);
		}
	};

	// One composition per scene, logged when it changes.
	size_t loggedInstances = (size_t)-1, loggedBatches = (size_t)-1, loggedGroups = (size_t)-1;
}

// ------------------------------------------------------------ per-frame list

void Scene3D::BuildGpuDrawList(const Renderer& renderer)
{
	frameActive = false;
	frameNumber++;
	slot = (slot + 1) % kSlots;
	viewIndex = 0;
	instances.clear();
	instanceModels.clear();
	batches.clear();
	batchTable.clear();
	batchCommands.clear();
	for (int s = 0; s < 2; s++)
	{
		templates[s].clear();
		groups[s].clear();
	}
	colourModels.clear();
	casterModels.clear();
	cpuColourModels.clear();
	cpuCasterModels.clear();

#ifdef USE_ASSIMP
	if (!active || renderer.camera.useOrthoCamera || shader == nullptr || !GpuDrivenWanted())
		return;
	// A game's own (older) scene3d copy keeps its own path: the GPU programs
	// pair the engine's vertex shaders with the scene's fragment shader.
	if (!ProgramHasBlock(shader->GetID(), "Material") || !EnsurePrograms())
		return;
	if (!MeshPoolVertexArray())
		return;   // no pooled meshes

	if (transforms.size() > models.size() * 2 + 256)
		transforms.clear();   // deleted models' entries pile up

	// Batches in first-appearance order: models sharing meshes, texture and material.
	std::unordered_map<BatchKey, uint32_t, BatchKeyHash> batchOf;
	std::vector<std::vector<std::pair<const Scene3DModel*, uint32_t>>> members;
	std::vector<uint8_t> modelFlags(models.size(), 0);
	for (size_t mi = 0; mi < models.size(); mi++)
	{
		const Scene3DModel* m = models[mi];
		if (m == nullptr || !m->loaded || m->texture == nullptr || m->guardHidden || m->IsWater())
			continue;
		const std::vector<Mesh*>& meshes = m->model3D.meshList;
		if (meshes.empty())
			continue;
		bool pooled = true;
		for (const Mesh* mesh : meshes)
		{
			MeshPoolRange range;
			if (mesh == nullptr || !MeshPoolFind(mesh, range))
			{
				pooled = false;
				break;
			}
		}
		if (!pooled)
			continue;

		const bool own = HasModelMaterials(meshes);
		uint32_t flags = 0;
		// As Scene3DModel::Render would draw it (the entity loop skips inactive ones).
		if (m->active && m->shader == shader && !(m->material != nullptr && m->material->IsTransparent() && !own))
			flags |= kFlagColour;
		// As the shadow passes pick casters: real height (flat ground doesn't cast).
		if (std::fabs(m->aabbMax.y - m->aabbMin.y) >= 15.0f)
			flags |= kFlagCaster;
		if (flags == 0)
			continue;

		const BatchKey key = { meshes[0], meshes.size(), m->texture, m->material };
		auto it = batchOf.find(key);
		uint32_t b;
		if (it == batchOf.end())
		{
			b = (uint32_t)batches.size();
			batchOf.emplace(key, b);
			batches.push_back(Batch());
			batches.back().leader = m;
			members.emplace_back();
		}
		else
		{
			b = it->second;
		}
		members[b].emplace_back(m, flags);
		modelFlags[mi] = (uint8_t)flags;
		if (flags & kFlagColour)
			colourModels.insert(m);
		if (flags & kFlagCaster)
			casterModels.insert(m);
	}
	if (batches.empty())
		return;
	for (size_t mi = 0; mi < models.size(); mi++)
	{
		if (!(modelFlags[mi] & kFlagColour))
			cpuColourModels.push_back(models[mi]);
		if (!(modelFlags[mi] & kFlagCaster))
			cpuCasterModels.push_back(models[mi]);
	}

	// Instances, batch by batch, each batch's models in scene order.
	for (uint32_t b = 0; b < (uint32_t)batches.size(); b++)
	{
		batches[b].firstInstance = (uint32_t)instances.size();
		batches[b].instanceCount = (uint32_t)members[b].size();
		for (const auto& entry : members[b])
		{
			const CachedTransform& t = TransformOf(entry.first);
			InstanceRecord r;
			r.model = t.model;
			r.sphere = t.sphere;
			r.info = glm::uvec4(b, entry.second, 0u, 0u);
			instances.push_back(r);
			instanceModels.push_back(entry.first);
		}
	}

	// Commands: one per batch mesh and set, gathered by draw state so each
	// group of consecutive commands is one multi-draw.
	struct Pending { uint32_t group, batch; MeshPoolRange range; };
	std::vector<Pending> pending[2];
	std::unordered_map<ColourKey, uint32_t, ColourKeyHash> colourGroup;
	std::unordered_map<Texture*, uint32_t> depthGroup;
	for (uint32_t b = 0; b < (uint32_t)batches.size(); b++)
	{
		const Scene3DModel* leader = batches[b].leader;
		for (const Mesh* mesh : leader->model3D.meshList)
		{
			MeshPoolRange range;
			MeshPoolFind(mesh, range);
			const ModelMaterial* own = MeshMaterial(mesh);

			// Colour: blended glTF meshes draw in the transparent pass instead.
			if (own == nullptr || own->alphaMode != AlphaMode::Blend)
			{
				const ColourKey key = own != nullptr ? ColourKey{ nullptr, nullptr, own }
					: ColourKey{ leader->texture, leader->material, nullptr };
				auto it = colourGroup.find(key);
				if (it == colourGroup.end())
				{
					it = colourGroup.emplace(key, (uint32_t)groups[kColourSet].size()).first;
					Group g;
					g.texture = key.texture;
					g.material = key.material;
					g.own = key.own;
					groups[kColourSet].push_back(g);
				}
				pending[kColourSet].push_back({ it->second, b, range });
			}

			// Depth: alpha cut-outs by the texture the CPU path binds.
			Texture* alpha = (own != nullptr) ? ModelShadowAlpha(*own) : leader->texture;
			auto dt = depthGroup.find(alpha);
			if (dt == depthGroup.end())
			{
				dt = depthGroup.emplace(alpha, (uint32_t)groups[kDepthSet].size()).first;
				Group g;
				g.texture = alpha;
				groups[kDepthSet].push_back(g);
			}
			pending[kDepthSet].push_back({ dt->second, b, range });
		}
	}

	std::vector<std::vector<uint32_t>> commandsOf[2];
	for (int s = 0; s < 2; s++)
	{
		commandsOf[s].assign(batches.size(), std::vector<uint32_t>());
		std::vector<uint32_t> fill(groups[s].size(), 0);
		for (const Pending& p : pending[s])
			groups[s][p.group].commandCount++;
		uint32_t start = 0;
		for (Group& g : groups[s])
		{
			g.firstCommand = start;
			start += g.commandCount;
		}
		templates[s].resize(start);
		for (const Pending& p : pending[s])
		{
			const uint32_t index = groups[s][p.group].firstCommand + fill[p.group]++;
			DrawIndexedIndirectCommand& c = templates[s][index];
			c.indexCount = p.range.indexCount;
			c.instanceCount = 0;
			c.firstIndex = p.range.firstIndex;
			c.baseVertex = p.range.baseVertex;
			c.baseInstance = batches[p.batch].firstInstance;   // + the view's region, per view
			commandsOf[s][p.batch].push_back(index);
		}
	}

	batchTable.resize(batches.size() * 6);
	for (uint32_t b = 0; b < (uint32_t)batches.size(); b++)
	{
		batchTable[b * 6 + 0] = batches[b].firstInstance;
		batchTable[b * 6 + 1] = batches[b].instanceCount;
		for (int s = 0; s < 2; s++)
		{
			batchTable[b * 6 + 2 + s] = (uint32_t)batchCommands.size();
			batchTable[b * 6 + 4 + s] = (uint32_t)commandsOf[s][b].size();
			batchCommands.insert(batchCommands.end(), commandsOf[s][b].begin(), commandsOf[s][b].end());
		}
	}

	// Upload this frame's list.
	RenderDevice& device = Device();
	Slot& sl = slots[slot];
	device.UpdateBuffer(Ensure(sl.instances, instances.size() * sizeof(InstanceRecord)), 0,
		instances.size() * sizeof(InstanceRecord), instances.data());
	device.UpdateBuffer(Ensure(sl.batches, batchTable.size() * sizeof(uint32_t)), 0,
		batchTable.size() * sizeof(uint32_t), batchTable.data());
	device.UpdateBuffer(Ensure(sl.lists, std::max<size_t>(batchCommands.size(), 1) * sizeof(uint32_t)), 0,
		batchCommands.size() * sizeof(uint32_t), batchCommands.empty() ? nullptr : batchCommands.data());
	frameActive = true;

	if (instances.size() != loggedInstances || batches.size() != loggedBatches
		|| groups[kColourSet].size() + groups[kDepthSet].size() != loggedGroups)
	{
		loggedInstances = instances.size();
		loggedBatches = batches.size();
		loggedGroups = groups[kColourSet].size() + groups[kDepthSet].size();
		std::cout << "GPU-driven models: " << instances.size() << " instances in " << batches.size()
			<< " batches; " << groups[kColourSet].size() << " colour + " << groups[kDepthSet].size()
			<< " depth multi-draws (" << templates[kColourSet].size() << " + " << templates[kDepthSet].size()
			<< " commands)" << std::endl;
		// KINJO_GPU_DRIVEN_DUMP=1: every batch, with its instances' flags.
		const char* dump = std::getenv("KINJO_GPU_DRIVEN_DUMP");
		if (dump != nullptr && dump[0] == '1')
		{
			for (uint32_t b = 0; b < (uint32_t)batches.size(); b++)
			{
				std::cout << "  batch " << b << ": " << batches[b].leader->objPath << " / "
					<< batches[b].leader->texPath << " x" << batches[b].instanceCount << " flags";
				for (uint32_t i = 0; i < batches[b].instanceCount; i++)
					std::cout << " " << instances[batches[b].firstInstance + i].info.y;
				std::cout << std::endl;
			}
		}
	}
#else
	(void)renderer;
#endif
}

// ------------------------------------------------------------ one view

namespace
{
	// Cull this frame's instances for one view: copies `set`'s commands into
	// the view's region, compacts the visible instances, fills the counts.
	// Returns the view's first command (in records), or -1 if there's nothing.
	long CullView(int set, const glm::mat4& viewProj, uint32_t flags)
	{
		if (!frameActive || templates[set].empty())
			return -1;
		RenderDevice& device = Device();
		Slot& sl = slots[slot];
		const size_t perViewCommands = std::max(templates[0].size(), templates[1].size());
		const size_t perViewVisible = instances.size();
		const size_t needCommands = (viewIndex + 1) * perViewCommands * sizeof(DrawIndexedIndirectCommand);
		const size_t needVisible = (viewIndex + 1) * perViewVisible * sizeof(uint32_t);
		if (sl.commands.capacity < needCommands || sl.visible.capacity < needVisible)
		{
			// Grown buffers start empty: views already drawn this frame used the old ones.
			Ensure(sl.commands, std::max(needCommands, perViewCommands * sizeof(DrawIndexedIndirectCommand) * 8));
			Ensure(sl.visible, std::max(needVisible, perViewVisible * sizeof(uint32_t) * 8));
		}
		const uint32_t commandBase = (uint32_t)(viewIndex * perViewCommands);
		const uint32_t visibleBase = (uint32_t)(viewIndex * perViewVisible);
		viewIndex++;

		static std::vector<DrawIndexedIndirectCommand> commands;
		commands = templates[set];
		for (DrawIndexedIndirectCommand& c : commands)
			c.baseInstance += visibleBase;
		device.UpdateBuffer(sl.commands.handle, commandBase * sizeof(DrawIndexedIndirectCommand),
			commands.size() * sizeof(DrawIndexedIndirectCommand), commands.data());

		glm::vec4 planes[6];
		FrustumPlanes(viewProj, planes);
		device.UseProgram(ProgramHandle(cullProgram));
		for (int p = 0; p < 6; p++)
			device.SetUniform(cullLoc.planes[p], planes[p]);
		device.SetUniform(cullLoc.commandSet, set);
		device.SetUniform(cullLoc.commandBase, (int)commandBase);
		device.SetUniform(cullLoc.visibleBase, (int)visibleBase);
		device.SetUniform(cullLoc.flags, (int)flags);
		device.SetUniform(cullLoc.batchCount, (int)batches.size());
		device.BindStorageBuffer(0, sl.instances.handle);
		device.BindStorageBuffer(2, sl.batches.handle);
		device.BindStorageBuffer(3, sl.lists.handle);
		device.BindStorageBuffer(4, sl.commands.handle);
		device.BindStorageBuffer(5, sl.visible.handle);
		const uint32_t groupsX = std::min<uint32_t>((uint32_t)batches.size(), 65535u);
		const uint32_t groupsY = ((uint32_t)batches.size() + 65534u) / 65535u;
		device.Dispatch(groupsX, groupsY);
		device.GpuBarrier(BarrierIndirect | BarrierVertexAttributes | BarrierStorage);
		return (long)commandBase;
	}

	// The pool's vertex array, its per-instance index attribute pointed at this
	// frame's visible list, and the instance list bound for the vertex shaders.
	VertexArrayHandle BindDrawInputs()
	{
		RenderDevice& device = Device();
		Slot& sl = slots[slot];
		const VertexArrayHandle vao = MeshPoolVertexArray();
		device.SetVertexAttributeInt(vao, 3, sl.visible.handle, 1, sizeof(uint32_t), 0, 1);
		device.BindStorageBuffer(0, sl.instances.handle);
		return vao;
	}

	void MultiDraw(VertexArrayHandle vao, long commandBase, const Group& g)
	{
		Device().MultiDrawIndexedIndirect(vao, Primitive::Triangles, slots[slot].commands.handle,
			(size_t)(commandBase + g.firstCommand) * sizeof(DrawIndexedIndirectCommand), (int)g.commandCount,
			sizeof(DrawIndexedIndirectCommand));
	}
}

bool Scene3D::DrawGpuColourView(const Renderer& renderer, unsigned int program, bool world)
{
	if (!frameActive || colourModels.empty())
		return false;
	const long commandBase = CullView(kColourSet, renderer.camera.projection * renderer.camera.CalculateViewMatrix(),
		kFlagColour);
	if (commandBase < 0)
		return false;

	RenderDevice& device = Device();
	const VertexArrayHandle vao = BindDrawInputs();
	device.UseProgram(ProgramHandle(program));
	renderer.BindWorldCameraBlock();
	device.SetUniform(device.UniformLocation(ProgramHandle(program), "theTexture"), 0);

	unsigned int buffers = 1u;
	if (world)
	{
		// How far each model moved since last frame (motion vectors, TAA).
		buffers = Scene3DInternal::WorldDrawBuffers(program, false);
		static std::vector<glm::vec4> motion;
		motion.assign(instances.size(), glm::vec4(0.0f));
		if (buffers & 4u)
		{
			for (size_t i = 0; i < instanceModels.size(); i++)
				motion[i] = glm::vec4(Scene3DInternal::MotionOffset(instanceModels[i], instanceModels[i]->position), 0.0f);
		}
		Slot& sl = slots[slot];
		device.UpdateBuffer(Ensure(sl.motion, motion.size() * sizeof(glm::vec4)), 0,
			motion.size() * sizeof(glm::vec4), motion.data());
		device.BindStorageBuffer(1, sl.motion.handle);
		ApplyLighting(program, renderer);
		if (buffers != 1u)
			device.SetBoundDrawBufferMask(buffers);
	}

	for (const Group& g : groups[kColourSet])
	{
		if (g.commandCount == 0)
			continue;
		if (g.own != nullptr)
		{
			Scene3DInternal::ApplyModelMaterial(program, *g.own);
		}
		else
		{
			ApplyMaterial(program, g.material ? *g.material : MaterialLibrary::Get().Default(), nullptr);
			g.texture->UseTexture();
		}
		MultiDraw(vao, commandBase, g);
		renderer.drawCallsPerFrame++;
	}

	if (buffers != 1u)
		device.SetBoundDrawBufferMask(1u);
	return true;
}

bool Scene3D::DrawGpuDepthView(const glm::mat4& viewProj, unsigned int program)
{
	if (!frameActive || casterModels.empty())
		return false;
	const long commandBase = CullView(kDepthSet, viewProj, kFlagCaster);
	if (commandBase < 0)
		return false;
	RenderDevice& device = Device();
	const VertexArrayHandle vao = BindDrawInputs();
	device.UseProgram(ProgramHandle(program));
	device.SetUniform(device.UniformLocation(ProgramHandle(program), "theTexture"), 0);
	for (const Group& g : groups[kDepthSet])
	{
		if (g.commandCount == 0)
			continue;
		if (g.texture != nullptr)
			g.texture->UseTexture();
		MultiDraw(vao, commandBase, g);
	}
	return true;
}

void Scene3D::DrawGpuDrivenModels(const Renderer& renderer)
{
	// The first GPU-driven model to Render this frame draws them all.
	if (!frameActive || worldDrawnFrame == frameNumber)
		return;
	worldDrawnFrame = frameNumber;
	DrawGpuColourView(renderer, worldProgram->GetID(), true);
}

// ------------------------------------------------------------ queries

bool Scene3DInternal::GpuDrivenFrame()
{
	return frameActive;
}

void Scene3DInternal::ResetGpuWorldDraw()
{
	worldDrawnFrame = frameNumber - 1;   // not drawn this frame
}

bool Scene3DInternal::GpuDrawnColour(const Scene3DModel* model)
{
	return frameActive && colourModels.count(model) > 0;
}

bool Scene3DInternal::GpuDrawnCaster(const Scene3DModel* model)
{
	return frameActive && casterModels.count(model) > 0;
}

const std::vector<Scene3DModel*>& Scene3DInternal::CpuColourModels(const std::vector<Scene3DModel*>& all)
{
	return frameActive ? cpuColourModels : all;
}

const std::vector<Scene3DModel*>& Scene3DInternal::CpuCasterModels(const std::vector<Scene3DModel*>& all)
{
	return frameActive ? cpuCasterModels : all;
}

unsigned int Scene3DInternal::GpuShadowProgram(bool point)
{
	if (!programsOk)
		return 0;
	return point ? pointProgram->GetID() : shadowProgram->GetID();
}

unsigned int Scene3DInternal::GpuPrepassProgram()
{
	return programsOk ? prepassProgram->GetID() : 0;
}

// ------------------------------------------------------------ KINJO_HIZ_STATS
// How much a Hi-Z occlusion test would cull (docs/RENDERING_NEXT_STEPS.md):
// after the world pass, the frame's final depth becomes a Hi-Z pyramid
// (render/HiZ.h) and shaders/hiz_stats.comp tests every in-view GPU-driven
// model against it. Counts are read back a few frames late (no stall) and
// averaged. Measures only; nothing is culled.

namespace
{
	int hizStatsEvery = -1;   // -1 = not read yet, 0 = off, else report every N frames
	unsigned int statsProgram = 0;
	bool statsTried = false;
	struct { int viewProj, width, height, levels, instanceCount, hiz; } statsLoc;
	const int kStatsRing = 4;
	BufferHandle statsCounts[kStatsRing];
	bool statsWritten[kStatsRing] = {};
	int statsSlot = 0;
	Buffer statsTriangles;
	// hiz_stats.comp's counts: models in view, hidden (coarse), hidden (fine),
	// then the same three in triangles.
	const int kStatCount = 6;
	uint64_t sums[kStatCount] = {};
	int statsFrames = 0;
}

bool Scene3DInternal::HizStatsWanted()
{
	if (hizStatsEvery < 0)
	{
		const char* v = std::getenv("KINJO_HIZ_STATS");
		const int n = (v != nullptr) ? std::atoi(v) : 0;
		hizStatsEvery = (n == 1) ? 60 : std::max(n, 0);
		if (hizStatsEvery > 0)
			std::cout << "KINJO_HIZ_STATS: Hi-Z occlusion estimate every " << hizStatsEvery << " frames" << std::endl;
	}
	return hizStatsEvery > 0;
}

void Scene3DInternal::MeasureOcclusion(const Renderer& renderer, unsigned int depthTexture, int width, int height)
{
	if (!frameActive || colourModels.empty() || !HizStatsWanted())
		return;
	RenderDevice& device = Device();
	if (!statsTried)
	{
		statsTried = true;
		statsProgram = CreateComputeProgramFromFile("data/shaders/hiz_stats.comp");
		if (statsProgram != 0)
		{
			const ProgramHandle p(statsProgram);
			statsLoc.viewProj = device.UniformLocation(p, "draw.viewProj");
			statsLoc.width = device.UniformLocation(p, "draw.width");
			statsLoc.height = device.UniformLocation(p, "draw.height");
			statsLoc.levels = device.UniformLocation(p, "draw.levels");
			statsLoc.instanceCount = device.UniformLocation(p, "draw.instanceCount");
			statsLoc.hiz = device.UniformLocation(p, "hiz");
			for (BufferHandle& b : statsCounts)
				b = device.CreateBuffer(8 * sizeof(uint32_t), nullptr, BufferUsage::Dynamic);
		}
	}
	if (statsProgram == 0 || !BuildHiZ(TextureHandle(depthTexture), width, height))
		return;

	// The counts written kStatsRing - 1 frames ago (the GPU is done with them).
	const int readSlot = (statsSlot + 1) % kStatsRing;
	if (statsWritten[readSlot])
	{
		uint32_t c[8] = {};
		device.ReadBuffer(statsCounts[readSlot], 0, sizeof(c), c);
		for (int k = 0; k < kStatCount; k++)
			sums[k] += c[k];
		if (++statsFrames >= hizStatsEvery)
		{
			double avg[kStatCount];
			for (int k = 0; k < kStatCount; k++)
				avg[k] = (double)sums[k] / statsFrames;
			auto pct = [](double part, double whole) { return whole > 0.0 ? 100.0 * part / whole : 0.0; };
			char line[384];
			snprintf(line, sizeof(line), "Hi-Z estimate (camera vs the frame's final depth, avg of %d frames): "
				"%.0f in-view models, %.0f triangles. Hidden - culler's 2x2 test: %.0f models (%.1f%%), "
				"%.1f%% of triangles; finer test: %.0f models (%.1f%%), %.1f%% of triangles",
				statsFrames, avg[0], avg[3], avg[1], pct(avg[1], avg[0]), pct(avg[4], avg[3]),
				avg[2], pct(avg[2], avg[0]), pct(avg[5], avg[3]));
			std::cout << line << std::endl;
			for (uint64_t& sum : sums)
				sum = 0;
			statsFrames = 0;
		}
	}

	// Each batch's triangles in the colour passes (blended glTF meshes draw elsewhere).
	static std::vector<uint32_t> triangles;
	triangles.assign(batches.size(), 0u);
	for (size_t b = 0; b < batches.size(); b++)
	{
		for (const Mesh* mesh : batches[b].leader->model3D.meshList)
		{
			const ModelMaterial* own = MeshMaterial(mesh);
			MeshPoolRange range;
			if ((own == nullptr || own->alphaMode != AlphaMode::Blend) && MeshPoolFind(mesh, range))
				triangles[b] += range.indexCount / 3;
		}
	}
	device.UpdateBuffer(Ensure(statsTriangles, triangles.size() * sizeof(uint32_t)), 0,
		triangles.size() * sizeof(uint32_t), triangles.data());

	const uint32_t zero[8] = {};
	device.UpdateBuffer(statsCounts[statsSlot], 0, sizeof(zero), zero);
	device.UseProgram(ProgramHandle(statsProgram));
	device.SetUniform(statsLoc.viewProj, renderer.camera.projection * renderer.camera.CalculateViewMatrix());
	device.SetUniform(statsLoc.width, width);
	device.SetUniform(statsLoc.height, height);
	device.SetUniform(statsLoc.levels, HiZLevels());
	device.SetUniform(statsLoc.instanceCount, (int)instances.size());
	device.BindTexture(0, HiZTexture());
	device.SetUniform(statsLoc.hiz, 0);
	device.BindStorageBuffer(0, slots[slot].instances.handle);
	device.BindStorageBuffer(6, statsCounts[statsSlot]);
	device.BindStorageBuffer(7, statsTriangles.handle);
	device.Dispatch((unsigned int)((instances.size() + 63) / 64));
	device.GpuBarrier(BarrierStorage | BarrierBufferRead);
	statsWritten[statsSlot] = true;
	statsSlot = (statsSlot + 1) % kStatsRing;
}
