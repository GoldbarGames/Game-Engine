#ifndef SCENE3D_INTERNAL_H
#define SCENE3D_INTERNAL_H
#pragma once

// Engine-internal helpers shared by the Scene3D*.cpp files (Scene3D,
// Scene3DLighting, Scene3DShadows, Scene3DWeather). Not exported.

#include "render/RenderDevice.h"
#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Game;
class Texture;
class Renderer;
class Mesh;
class Scene3DModel;
struct ModelMaterial;

namespace Scene3DInternal
{
	// Colour art the scene renders lit (model albedo, character billboards,
	// seasonal swaps): with the linear workflow on, an sRGB copy so the lit
	// shaders sample linear values (render/ColorPipeline.h); otherwise the
	// plain cached image, exactly as before. Normal maps must NOT come from here.
	Texture* SceneColorTexture(const Game& game, const std::string& path);

	// The depth-only passes' draw of a model's meshes: alpha cut-outs by
	// `modelTexture`, or per mesh by its own (glTF) material's alpha mode.
	void DrawMeshesForDepth(const std::vector<Mesh*>& meshes, Texture* modelTexture);

	// --- cascaded sun shadows (Scene3DShadows.cpp) -------------------------
	//   renderer.dat  `shadowCascades <0-4>` (default 4; 0 = the single sun map)
	//                 `shadowDistance <world units>` (default 5000)
	//                 `shadowSoftness <texels>` (PCF spacing, default 1)
	//   .scene        `shadowdistance <world units>` - that scene's own reach
	// Used when the scene's model shader declares the "Cascades" block (the
	// engine's scene3d.frag); an older game copy keeps the single sun map.
	// Per lit draw: the Cascades block, the sampler's unit, and the maps when active.
	void BindCascades(unsigned int program);
	void SetSceneShadowDistance(float distance);   // < 0 = project default
	float SceneShadowDistance();
	float ProjectShadowDistance();                 // renderer.dat `shadowDistance`
	void ReloadShadowSettings();                   // re-read renderer.dat's shadow keys

	// A scene's own cel-shading / outline settings. Games set Scene3D's
	// celShading and outline* fields in code; a scene may override any of them
	// (.scene `cel on|off`, `outline on|off`, `outlinechars on|off`,
	// `outlinewidth <px>`, `outlinedepth <v>`, `outlinecolor <r> <g> <b>`). The
	// game's values are kept meanwhile and put back when the next scene loads.
	enum class ToonSetting { CelShading, Outline, OutlineCharacters, OutlineWidth, OutlineDepth, OutlineColor, Count };
	void OwnToonSetting(ToonSetting s);      // call before changing the field: keeps the game's value
	bool SceneOwnsToonSetting(ToonSetting s);
	void ResetToonSetting(ToonSetting s);    // back to the game's value
	void RestoreGameToonSettings();          // all of them (scene load / unload)

	// A .scene file's comments (and lines the engine doesn't recognise) are kept
	// from loading to saving, each with the data line it was above. A renamed
	// light, camera or slot keeps its own: kind "point", "spot", "camera" or "slot".
	void RenameSceneLine(const std::string& kind, const std::string& from, const std::string& to);

	// Engine shaders declare the uniform blocks; old copies kept in a game's
	// data/shaders predate them and still need the loose uniforms.
	inline bool ProgramHasBlock(unsigned int program, const char* name)
	{
		return Device().HasUniformBlock(ProgramHandle(program), name);
	}

	// --- motion vectors for temporal anti-aliasing (Scene3D.cpp) -------------
	// The world target's draw buffers for a lit draw with `program`: colour,
	// plus the character mask when asked, plus the motion attachment while the
	// world pass writes motion (render/TemporalAA.h) and the program has the
	// "Motion" block (a game's own model shader might not, and an enabled
	// attachment it doesn't write would receive garbage). 1 = colour only, the
	// world pass's default - restore that after the draw.
	unsigned int WorldDrawBuffers(unsigned int program, bool characterMask);
	// How far `object` (any stable pointer) moved since last frame: its
	// position now minus then, or 0 if it wasn't drawn last frame. Only while
	// motion is written; repeated calls within a frame return the same offset.
	glm::vec3 MotionOffset(const void* object, const glm::vec3& position);
	void ForgetMotion();   // the scene unloaded

	// --- materials imported with a model (glTF; ModelMaterials.h) -----------
	// The Material block with the material's map bits, and its maps: base
	// colour on unit 0, metallic-roughness 2, occlusion 5, emissive 6 (5 and 6
	// only where KINJO_GL4 frees them), normal map 1.
	void ApplyModelMaterial(unsigned int program, const ModelMaterial& material);

	// --- GPU-driven models (Scene3DGpuDriven.cpp, Phase 1.5 item 11) ---------
	//   renderer.dat `gpuDriven 1|0` (default 1); KINJO_GPU_DRIVEN=0/1 for a run.
	bool GpuDrivenFrame();                            // this frame's models go through it
	bool GpuDrawnColour(const Scene3DModel* model);   // its world / AO-prepass draws are the GPU path's
	bool GpuDrawnCaster(const Scene3DModel* model);   // its shadow draws are
	// The models a CPU loop still has to consider: `all` minus those the GPU
	// path draws this frame (or `all` itself when it isn't active).
	const std::vector<Scene3DModel*>& CpuColourModels(const std::vector<Scene3DModel*>& all);
	const std::vector<Scene3DModel*>& CpuCasterModels(const std::vector<Scene3DModel*>& all);
	unsigned int GpuShadowProgram(bool point);        // the GPU path's depth programs (sun / cube face)
	unsigned int GpuPrepassProgram();
	// KINJO_HIZ_STATS=1 (or =<frames>): after the world pass, estimate how many
	// in-view models a Hi-Z occlusion test against the frame's depth would cull
	// (docs/RENDERING_NEXT_STEPS.md). Measures only. Needs the GPU-driven path.
	bool HizStatsWanted();
	void MeasureOcclusion(const Renderer& renderer, unsigned int depthTexture, int width, int height);
	// A shadow pass's values for a bound depth program (Scene3DShadows.cpp).
	void ApplyShadowPass(unsigned int program, const glm::mat4& viewProj, const glm::vec3& lightPos,
		float farPlane, float alphaCutoff, bool sun);
}

#endif
