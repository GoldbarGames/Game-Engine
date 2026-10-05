#ifndef SCENE3D_H
#define SCENE3D_H
#pragma once

#include "Entity.h"
#include "Model.h"
#include "SceneMaterial.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include <iosfwd>

class Game;
class Texture;
class ShaderProgram;
class Mesh;
class Skybox;

// Optional directional fill light (the .scene "light" line). Diffuse 0
// leaves it off - a point/spot-lit room usually wants only a faint fill.
struct SceneDirLight
{
	glm::vec3 dir = glm::vec3(0.35f, 1.0f, 0.25f);  // points from light into scene
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	float diffuse = 0.0f;
};

// Runtime intensity fade (a light animating from one intensity to another
// over a number of seconds); shared by point and spot lights.
struct LightFade
{
	bool active = false;
	float from = 0.0f;
	float to = 0.0f;
	float elapsed = 0.0f;
	float duration = 0.0f;
};

// Point light (.scene "point" line): omni glow with finite range.
struct ScenePointLight
{
	std::string name;          // for runtime control ("scene3d light <name> ...")
	bool on = true;
	glm::vec3 pos = glm::vec3(0, 0, 0);
	glm::vec3 color = glm::vec3(1, 1, 1);
	float range = 300.0f;      // world units to full falloff
	float intensity = 1.0f;
	LightFade fade;
	// Strobe (emergency lights / alarms): flashHz > 0 blinks the light on/off that
	// many times per second; flashPhase (0..1) offsets the cycle - pair a red light
	// at phase 0 with a blue one at phase 0.5 to alternate. flashPeak holds the
	// authored intensity so the strobe restores it each "on".
	float flashHz = 0.0f;
	float flashPhase = 0.0f;
	float flashPeak = 1.0f;
	// Availability guard (.scene "if <guard>"): when set, ObjectGuards toggles
	// guardHidden each frame so the light turns off with its object (e.g. a
	// police car that only appears once a story flag is set).
	std::string guard;
	bool guardHidden = false;
};

// Spot light (.scene "spot" line): a cone. Angles are half-angles in deg.
struct SceneSpotLight
{
	std::string name;
	bool on = true;
	glm::vec3 pos = glm::vec3(0, 0, 0);
	glm::vec3 dir = glm::vec3(0, 1, 0);  // cone axis (points where it shines)
	glm::vec3 color = glm::vec3(1, 1, 1);
	float range = 500.0f;
	float intensity = 1.0f;
	float innerDeg = 18.0f;    // full-bright inside this half-angle
	float outerDeg = 30.0f;    // fades to 0 by this half-angle
	LightFade fade;
};

// Per-surface tuning for a water model (only used when the model's material is
// the Water lighting mode). Editable live in the 3D editor (the WATER button)
// and serialized to the .scene "water ..." token. Overrides the material's
// specular/shininess/opacity so several lakes can share one water material yet
// look different.
struct WaterSurface
{
	float amplitude = 9.0f;    // vertex wave height (world units); 0 = flat
	float waveScale = 1.0f;    // spatial frequency (higher = smaller, tighter waves)
	float shoreFade = 0.35f;   // 0 = waves reach the edge; 1 = calm at the shoreline
	float choppy    = 1.0f;    // fragment ripple-normal strength (surface detail)
	float specular  = 0.6f;    // sun-glint strength
	float shininess = 120.0f;  // glint tightness
	float opacity   = 0.5f;    // 1 = opaque; lower = more see-through
};

// One placed 3D model inside a Scene3D (loaded from a .scene file line).
// Renders through the dedicated scene3d shader with a single texture -
// the model's own material list is ignored so scene files stay simple.
class KINJO_API Scene3DModel : public Entity
{
public:
	Model model3D;
	bool loaded = false;
	Texture* texture = nullptr;        // owned by the sprite manager cache
	ShaderProgram* shader = nullptr;   // owned by Scene3D
	// Rotation is Euler (degrees): yaw about the vertical (-Y) axis is the
	// common case and lives in the .scene main field; pitch (X) and roll (Z)
	// are optional (serialized as a "rot" token only when non-zero).
	float yawDeg = 0.0f;
	float pitchDeg = 0.0f;
	float rollDeg = 0.0f;
	// Uniform base scale (the .scene <scale> field) times an optional per-axis
	// multiplier (serialized as "scaleaxis" only when not 1,1,1). Effective
	// scale = modelScale * scaleAxis, component-wise.
	float modelScale = 1.0f;
	glm::vec3 scaleAxis = glm::vec3(1.0f);
	glm::vec3 EffectiveScale() const { return glm::vec3(modelScale) * scaleAxis; }

	// Source spec (kept so the editor can re-serialize the .scene file) and
	// a world-space AABB (recomputed on load / when moved) for ray-picking.
	std::string objPath, texPath;
	// Runtime scratch: this model currently has its winter bare-branch mesh loaded
	// (deciduous material in winter) instead of its authored objPath mesh. Not
	// serialized - objPath keeps the authored (summer) mesh for the .scene file.
	bool bareMesh = false;
	bool solid = false;
	// Walkable top: characters can stand on (and step/fall onto) this model's
	// upper surface. Authored via the .scene "walk" flag. Powers
	// Scene3D::GetGroundHeight for floors, stairs and multi-level rooms.
	bool walkable = false;
	// Game-agnostic interaction tag: an arbitrary label a game can attach to a
	// model (a clue id, a door name, a pickup type, ...). Authored via the
	// .scene "model ... tag <VALUE>" field or the editor's TAG button. The
	// engine only stores and serializes it; games decide what it means.
	std::string interactionTag;
	bool tagTriggered = false;    // runtime scratch: game flag (e.g. already used)
	// Availability guard (the .scene "if <expr>" token): a game-evaluated condition
	// (world-state flags / choices / time). The engine only stores it; a game system
	// evaluates it each frame and sets guardHidden, and every render/pick pass skips
	// models where guardHidden is true. Empty guard = always shown.
	std::string guard;
	bool guardHidden = false;
	// Surface material (specular / normal map / lighting model / ...). Named via
	// the .scene "mat <name>" token, resolved to a MaterialLibrary entry on
	// load. null = the library's default matte material.
	std::string materialName;
	const SceneMaterial* material = nullptr;
	// Per-surface water tuning (only meaningful when material is the Water mode).
	WaterSurface water;
	bool IsWater() const { return material != nullptr && material->lighting == LightingModel::Water; }
	glm::vec3 localMin = glm::vec3(0), localMax = glm::vec3(0); // OBJ-space bounds
	bool hasLocalBounds = false;
	glm::vec3 aabbMin = glm::vec3(0), aabbMax = glm::vec3(0);   // world-space, for picking

	// Instanced-draw grouping (rebuilt each frame by Scene3D::RebuildInstanceGroups):
	// duplicate opaque props sharing obj+texture+material draw in one instanced call.
	// leader: index into Scene3D::instanceGroups; member: drawn by its leader (skip).
	int instanceGroupIndex = -1;
	bool instanceMember = false;

	// The full model transform (translate * yaw * pitch * roll * scale).
	glm::mat4 ModelMatrix() const;

	Scene3DModel(const glm::vec3& pos);

	void Update(Game& game) override;
	void Render(const Renderer& renderer) override;
	// The actual GL draw (all state/uniforms + mesh). Render() calls this for
	// opaque models; Scene3D's transparent pass calls it directly for the
	// depth-sorted transparent ones.
	void DrawGeometry(const Renderer& renderer);
};

// A VN character standing in the 3D scene: an upright billboard that yaws
// to face the camera (never tilts when the camera pitches). Built either
// from a layered body+head pair (the current asset structure) or a single
// combined sprite (fallback for future flat sprites).
class KINJO_API Character3D : public Entity
{
public:
	std::string charName;         // for "scene3d focus <name>" / future control
	Texture* bodyTex = nullptr;   // combined sprite goes here when unlayered
	Texture* headTex = nullptr;   // null for combined sprites
	ShaderProgram* shader = nullptr;
	Mesh* quad = nullptr;         // shared unit billboard quad (owned by Scene3D)
	float worldHeight = 220.0f;   // billboard height in world units
	float collisionRadius = 35.0f;  // base footprint pushed out of solids

	// Visible figure extent as fractions of the billboard height measured
	// FROM THE BASE (0 = feet on the floor, 1 = top of the billboard),
	// derived from the sprite's opaque alpha bounds so closeup framing
	// adapts to each character's art (transparent margins differ per
	// sprite). Defaults assume the figure fills most of the canvas.
	float figureTopFrac = 0.97f;  // top of the head
	float figureBotFrac = 0.03f;  // feet
	// Horizontal center of the opaque region as a fraction from the canvas
	// left (0.5 = centered art). Used to center the DRAWN body horizontally
	// instead of the placement point, so sideways-offset art still centers.
	float figureHCenterFrac = 0.5f;

	// Source spec (for editor re-serialization). mode: "layered" or
	// "combined". For layered: folder/bodyPose/headExpr. For combined:
	// spritePath.
	std::string mode, folder, bodyPose, headExpr, spritePath;

	Character3D(const glm::vec3& pos);

	void Update(Game& game) override;
	void Render(const Renderer& renderer) override;

private:
	void DrawQuad(const Renderer& renderer, Texture* tex, float forwardBias);
};

// A 3D-rendered VN backdrop (Danganronpa style): a set of models plus
// NAMED camera positions, loaded from data/scenes/<name>.scene and driven
// by the "scene3d" cutscene command. While active, the main camera runs
// in perspective with depth testing; the VN textbox/GUI still draws on
// top through the screen-space GUI camera path.
//
// Scene file format (# comments allowed):
//   model  <objPath> <texturePath> <x> <y> <z> <yawDeg> <scale>
//   camera <name> <x> <y> <z> <pitchDeg> <yawDeg>
// The first camera listed is the one used on load.
class KINJO_API Scene3D
{
public:
	static Scene3D& Get();

	bool active = false;
	std::string currentScene;

	// Toon / cel-shading look: when true, scene models render with quantised
	// (banded) lighting and a solid inverted-hull outline. A game-wide style
	// toggle (default off); the shader still honours each material's albedo /
	// normal map / tint.
	bool celShading = false;
	bool outlineEnabled = true;                    // draw the post-process outline
	float outlineWidth = 3.0f;                     // outline thickness in pixels
	float outlineDepthThreshold = 4.0f;            // edge sensitivity (smaller = more edges)
	glm::vec3 outlineColor = glm::vec3(0.0f);      // solid black by default
	// Whether character sprites participate in the toon outline. When false, the
	// billboards render with depth WRITES off (still depth-TESTED, so scene models
	// still occlude them) so they don't appear in the depth texture the outline
	// reads - i.e. props get outlined, characters render as clean sprites. Only
	// matters while the outline is active (celShading && outlineEnabled).
	bool outlineCharacters = true;
	// Whether the scene's baked 3D characters render at all. Set false to use the
	// scene purely as a cutscene BACKDROP while the VN's own 2D character sprites do
	// the acting (otherwise a baked character and its 2D sprite would both show). Reset
	// to true on every Load. Toggled from script via `scene3d characters on|off`.
	bool renderCharacters = true;
	// When non-empty, ONLY the character with this charName renders (and casts
	// shadows) - a focused moment like a debate/cross-examination. Overrides
	// renderCharacters. Reset to "" on every Load.
	std::string soloCharacter = "";
	// Whether a given baked character should be drawn now (honours soloCharacter,
	// else renderCharacters). Used by the render + shadow passes.
	bool CharVisible(const Character3D* ch) const;

	// Runtime "focus spotlight" that lights one character (a debate / dramatic
	// confrontation). on=true aims a warm spot down onto <charName> from above and
	// toward the camera; on=false disables it. It's an EXTRA light on top of the
	// scene's own, reset off on Load.
	void SpotlightCharacter(Game& game, const std::string& charName, bool on);
	// Full-screen depth-edge outline shader (created on first scene load), run
	// by Game's composite pass. Public getter.
	// The post-process outline shader, built on first use.
	//
	// It used to be created only by Load, which meant the toon outline was
	// available exactly to games that loaded a .scene - and silently absent for
	// a game that draws its own world and asks for `celShading`. That is the
	// same trap the weather shader had (see EnsureWeatherResources), so it is
	// fixed the same way: the getter builds it.
	ShaderProgram* EdgeShader();

	// Shader files Scene3D loads for the 3D passes. A game may repoint these at
	// its own shaders (e.g. a realistic PBR set instead of the toon default)
	// BEFORE the first scene loads. The default files support both looks: leave
	// celShading off for smooth Phong/PBR, on for banded toon + outline.
	std::string modelShaderVert     = "data/shaders/scene3d.vert";
	std::string modelShaderFrag     = "data/shaders/scene3d.frag";
	std::string billboardShaderVert = "data/shaders/billboard3d.vert";
	std::string billboardShaderFrag = "data/shaders/billboard3d.frag";
	std::string instancedShaderVert = "data/shaders/scene3d_instanced.vert";
	std::string weatherShaderVert   = "data/shaders/weather.vert";
	std::string weatherShaderFrag   = "data/shaders/weather.frag";

	// Test harness: when true, close the game as soon as the test cutscene
	// ends (set by main.cpp in --test3d mode). Avoids relying on a script
	// quit command, which autoreturn skips on a label's last line.
	bool testAutoQuit = false;

	// If set, Scene3D::Update loads this scene once on its next tick. Used by
	// the interactive "--editscene <name>" launch to drop straight into a
	// scene for editing, with no cutscene involved.
	std::string autoLoadScene;

	struct CamPose
	{
		glm::vec3 position = glm::vec3(0, 0, 0);
		float pitch = 0.0f;
		float yaw = 90.0f;
	};

	// A named stand-point in the scene (the ".scene" "slot" token). The character
	// schedule places a character AT a named anchor rather than at raw coordinates,
	// so scenes describe WHERE someone can stand and the schedule decides WHO. Not
	// an entity - just data the loader/editor/schedule read.
	struct SceneAnchor
	{
		std::string name;
		glm::vec3 position = glm::vec3(0, 0, 0);
		float yaw = 0.0f;               // facing (degrees), for the placed character
	};

	// Load a scene file, spawn its models, switch to 3D rendering, and
	// jump to its first camera. Returns false if the file is missing.
	bool Load(Game& game, const std::string& sceneName);

	// In-memory serialize / deserialize (used by the editor's undo/redo and
	// dirty-state tracking). SerializeToString produces the same text SaveScene
	// writes; LoadFromString rebuilds the scene from such text. jumpCamera=false
	// leaves the editor's fly-camera where it is (undo shouldn't yank the view).
	std::string SerializeToString() const;
	bool LoadFromString(Game& game, const std::string& text,
		const std::string& sceneName, bool jumpCamera = false);

	// Remove the models and restore the VN's 2D ortho camera state
	void Unload(Game& game);

	// Draw transparent-material models (opacity < 1) back-to-front with depth
	// writes disabled. Call once, after the opaque 3D pass, while depth testing
	// is still enabled. No-op if the scene is inactive or has none.
	void RenderTransparentModels(Game& game, const Renderer& renderer);

	// --- shadow mapping (directional sun) -------------------------------
	// Real shadow maps: the scene depth is rendered from the sun's POV into a
	// depth texture, then the main scene + character shaders sample it to shade
	// occluded fragments. Casts onto arbitrary geometry (ground, walls, other
	// props) from both models and the standing character sprites. Skipped at
	// night (no sun). Call RenderShadowDepth once per frame BEFORE the main
	// scene render (Game::Render does).
	bool shadowsEnabled = true;
	float shadowStrength = 0.7f;    // how much shadowed areas darken (0..1)
	void RenderShadowDepth(Game& game, const Renderer& renderer);

	// Indoor point-light shadows: when there is no directional sun (a lamp-lit
	// room), the STRONGEST point light casts an omnidirectional (cube) shadow
	// map. One light only (cost). Call RenderPointShadowDepth once per frame
	// before the main pass (Game::Render does, alongside RenderShadowDepth).
	bool pointShadowsEnabled = true;
	// Which point light casts the shadow. Empty = AUTO (the enabled light with the
	// highest intensity*range). Otherwise the named light (if enabled). Editable in
	// the 3D editor and serialized as the .scene "shadowlight <name>" line.
	std::string shadowCasterLight;
	void RenderPointShadowDepth(Game& game, const Renderer& renderer);

	// Ambient occlusion (linear workflow; render/AmbientOcclusion.h): whether
	// this frame has a perspective scene whose model shader reads occlusion,
	// and the prepass drawing its opaque models into the occlusion targets.
	// Game::Render runs both, before the world.
	bool WantsScreenSpacePrepass(const Renderer& renderer) const;   // occlusion and/or reflections
	void RenderAoPrepass(Game& game, const Renderer& renderer);

	// Clustered lighting (render/ClusteredLights.h): sort this frame's enabled
	// point and spot lights - any number of them - into the camera's light
	// clusters, for the lit shaders. Game::Render runs it once, before the world.
	void UpdateLightClusters(Game& game, const Renderer& renderer);
	// Names of the scene's point lights, in order (for the editor's cycle button).
	std::vector<std::string> PointLightNames() const;
	// Mutable access to the point lights (ObjectGuards toggles their guardHidden).
	std::vector<ScenePointLight>& GetPointLights();
	// The spot lights and the sun, for the 3D editor's LIGHTS panel.
	std::vector<SceneSpotLight>& GetSpotLights();
	const SceneDirLight& GetDirectionalLight() const { return dirLight; }

	// Hard-cut the camera to a named pose. Returns false if unknown.
	bool JumpToCamera(Game& game, const std::string& camName);
	// Rename a saved camera (its place in the order stays). False if there's no
	// camera `from`, or one called `to` already.
	bool RenameCamera(const std::string& from, const std::string& to);

	// Start a smooth glide (smoothstep, shortest-path yaw) to a named pose.
	bool GlideToCamera(const std::string& camName, float seconds);

	// Smoothly glide the camera to an explicit pose (e.g. save the current view,
	// glide to a character closeup, then glide back to the saved pose).
	void GlideToPose(const CamPose& pose, float seconds);

	// Smoothly frame a named character. closeup=false frames the whole
	// figure centered; closeup=true frames an upper-body dialogue shot
	// (waist to just above the head), dollying the camera in/out along its
	// view axis to fit - the framing adapts to each character's proportions.
	// distanceOverride (> 0) forces a fixed camera distance instead.
	bool FocusCharacter(Game& game, const std::string& charName,
		float seconds, bool closeup = false, float distanceOverride = 0.0f);

	// Smoothly frame a world-space AABB (e.g. a prop being inspected): dolly the
	// camera in along its current horizontal view direction so the box fits the
	// view, aiming at the box center. fillFrac (0..1) is how much of the view the
	// box should fill (smaller = more margin around it). Stays on the camera's
	// current side (dolly, don't teleport around). Returns false if the box is
	// degenerate (zero size).
	bool FocusBounds(Game& game, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
		float seconds, float fillFrac = 0.7f);

	// Push a floor point (x,z; y ignored) out of every solid model's
	// footprint until it clears them all, treating the point as a circle of
	// the given radius. Reusable for future character movement. No-op if the
	// point is already clear or there are no solids.
	void ResolveAgainstSolids(glm::vec3& pos, float radius) const;

	// Y-aware variant: a solid only blocks when its vertical extent overlaps
	// the character's body span - feet at pos.y up to pos.y - bodyHeight
	// (up = -Y) - and its top rises more than maxStepUp above the feet
	// (lower tops are climbable steps, resolved by GetGroundHeight instead).
	void ResolveAgainstSolids(glm::vec3& pos, float radius,
		float bodyHeight, float maxStepUp) const;

	// Highest walkable surface at pos's XZ that is no more than maxStepUp
	// above the feet (any distance below counts - that's a fall). Returns
	// false if no walkable ground is under the point. Scenes with no
	// "walk"-flagged models always return false - callers keep their own
	// fallback (e.g. a flat y = 0).
	bool GetGroundHeight(const glm::vec3& pos, float maxStepUp, float& outY) const;

	// True when this scene has any "walk"-flagged surfaces (multi-level map);
	// false = flat scene, movement code keeps its own ground convention.
	bool HasWalkableGround() const { return !grounds.empty(); }

	// Advance an in-progress glide; call every frame while active.
	void Update(Game& game);

	// Lighting for one draw with a bound program: fills the "Scene" uniform
	// block (ambient, sun, lightning, point/spot lights, shadow casters,
	// camera position, time, toon) and binds the shadow-map textures. The
	// block is re-uploaded only when its contents change. Programs that
	// predate the block (old copies in a game's data/shaders) get the same
	// values as loose uniforms.
	void ApplyLighting(unsigned int shaderID, const Renderer& renderer) const;
	// Material for one draw: fills the "Material" uniform block (cached per
	// distinct material) and binds the normal map. A water model passes its
	// per-surface tuning; others pass nullptr. Same legacy fallback.
	void ApplyMaterial(unsigned int shaderID, const SceneMaterial& mat, const WaterSurface* water) const;

	// Refresh the shared camera UBO (view+projection, std140 binding 0) and
	// rebuild the per-frame instance groups. Call once per frame before the 3D
	// draws (Game::Render does). See scene3d.vert / scene3d_instanced.vert.
	void UpdateCameraUBO(const Renderer& renderer);

	// Draw one instance group (duplicate opaque props) in a single instanced
	// call. Called by the group leader's Scene3DModel::Render.
	void DrawInstancedGroup(const Renderer& renderer, int groupIndex);

	// Batch duplicate opaque props (same obj+texture+material) into one
	// instanced draw. On by default; toggle for A/B testing or debugging.
	bool instancingEnabled = true;

	// GPU-driven model rendering (Phase 1.5 item 11, Scene3DGpuDriven.cpp). On a
	// GL 4.3+ desktop context with the engine's scene3d shaders, the opaque
	// models come from one per-frame instance list: each view (the camera, a
	// shadow cascade, a cube face) is culled on the GPU and drawn with indirect
	// multi-draws, one per texture + material. Such a model's Render draws
	// nothing itself; the first one to Render in a frame draws them all.
	void DrawGpuDrivenModels(const Renderer& renderer);

	// Runtime light control (by the name given in the .scene file). All
	// return false if no light of that name exists.
	bool SetLightOn(const std::string& name, bool on);
	bool SetLightIntensity(const std::string& name, float intensity);
	bool FadeLightIntensity(const std::string& name, float target, float seconds);
	bool SetLightColor(const std::string& name, const glm::vec3& color);
	bool SetLightPosition(const std::string& name, const glm::vec3& pos);

	// Runtime model animation (`scene3d model <tag> turn|move ...`): every model
	// whose interaction tag is `tag` turns to a yaw (degrees) or moves to a
	// position over `seconds`, eased; 0 = at once. Returns how many models
	// matched. Loading or unloading a scene cancels animations in progress.
	int AnimateModelTurn(const std::string& tag, float yawDeg, float seconds);
	int AnimateModelMove(const std::string& tag, const glm::vec3& pos, float seconds);

	// --- environment / time-of-day control (runtime) ------------------
	// Ambient + directional fill can be driven every frame (e.g. by a game's
	// time-of-day system): ApplyLighting re-uploads them per frame, so the
	// scene reacts immediately. The sky tint MULTIPLIES the skybox panorama
	// (0..1 per channel, clamped); SetSkyTexture swaps the panorama itself.
	// Sky calls are no-ops when the scene has no sky. Neither touches the
	// authored .scene values, so editor saves keep the original sky/lights.
	void SetAmbientLight(const glm::vec3& c) { ambientColor = c; }
	glm::vec3 GetAmbientLight() const { return ambientColor; }
	void SetDirectionalLight(const glm::vec3& color, float diffuse)
	{
		dirLight.color = color;
		dirLight.diffuse = diffuse;
	}
	// Aim the sun (direction light travels into the scene; up is -Y). Driven by
	// a game's time-of-day system so shadows sweep + lengthen across the day.
	void SetSunDirection(const glm::vec3& dir)
	{
		if (glm::length(dir) > 1e-4f) dirLight.dir = glm::normalize(dir);
	}
	glm::vec3 GetSunDirection() const { return dirLight.dir; }
	bool HasSky() const { return skybox != nullptr; }
	void SetSkyTint(const glm::vec3& tint);
	void SetSkyTexture(Game& game, const std::string& path);
	// The scene's own panorama, as the .scene `sky` line saves it (the 3D
	// editor's sky picker). "" removes the sky; a scene without one gets it.
	void SetAuthoredSky(Game& game, const std::string& path);
	const std::string& GetAuthoredSky() const { return skyTexPath; }
	// The sky sphere's radius (the .scene `sky <png> <radius>`). It must stay
	// beyond the scene and inside the camera's far plane (drawing clamps it).
	float GetSkyRadius() const { return skyRadiusVal; }
	void SetSkyRadius(float radius);
	// Cross-fade the sky between two panoramas: blend 0 = fully fromPath,
	// 1 = fully toPath. Textures come from the sprite cache (cheap per frame).
	// Pass an empty toPath (or blend 0) for a single static panorama.
	void SetSkyCrossfade(Game& game, const std::string& fromPath,
		const std::string& toPath, float blend);

	// --- weather particles (rain / snow) ------------------------------
	// A GPU-instanced falling-particle volume that follows the camera. Rain =
	// fast blue-grey streaks along the fall direction; Snow = slow camera-facing
	// flakes with a sideways sway. Simulated in Update, drawn by RenderWeather
	// (after the opaque + transparent 3D passes, depth-tested, depth-write off).
	// Authored per scene via the "weather rain|snow|storm [intensity]" .scene token,
	// or set at runtime with SetWeather. The cloudy sky is a game-side concern
	// (DB2's TimeOfDaySky reacts to GetWeather()). Storm = heavy rain plus dynamic
	// lightning flashes (a full-screen additive flash) and delayed thunder audio.
	enum class WeatherType { None, Rain, Snow, Storm };
	void SetWeather(WeatherType type, float intensity = 1.0f);

	// World scale for the weather. The particle volume and the particle sizes
	// are both tuned for a scene a character walks around in; a game whose world
	// unit means something different needs them in proportion to ITS world or
	// the flakes come out metres across. 1 is the original look; TrainRails,
	// where one unit is one metre of railroad, uses about a tenth of it.
	float weatherScale = 1.0f;
	WeatherType GetWeather() const { return weatherType; }
	float GetWeatherIntensity() const { return weatherIntensity; }
	// Draw the weather particles. Call once per frame after RenderTransparentModels
	// (Game::Render does), while the perspective depth buffer is still bound.
	void RenderWeather(Game& game, const Renderer& renderer);

	// The storm's lightning flash: a full-screen additive pop, driven by the
	// same Update that drives the rain. Scene3D::Render calls this itself; a
	// game that draws its own world and only borrows the weather (TrainRails)
	// calls it after RenderWeather, or its storms have no lightning in them.
	void RenderLightningFlash(const Renderer& renderer);

	// --- fountain particle jet -----------------------------------------
	// A point emitter that sprays water droplets UP from `pos`, arcing back down
	// under gravity (a real fountain). Authored per scene via the ".scene" token
	// `fountain <x> <y> <z> [jetSpeed] [fallDist] [spread]`, or set at runtime.
	// Reuses the weather particle billboard shader for rendering.
	void SetFountain(const glm::vec3& pos, float jetSpeed = 480.0f,
		float fallDist = 175.0f, float spread = 60.0f);
	void ClearFountain() { hasFountain = false; }
	bool HasFountain() const { return hasFountain; }
	// Per-parameter access so the editor can tune the jet live.
	glm::vec3 GetFountainPos() const { return fountainPos; }
	void  SetFountainPos(const glm::vec3& p) { fountainPos = p; fountainInit = false; }
	float GetFountainJetSpeed() const { return fountainJetSpeed; }
	void  SetFountainJetSpeed(float v) { fountainJetSpeed = v; }
	float GetFountainFallDist() const { return fountainFallDist; }
	void  SetFountainFallDist(float v) { fountainFallDist = v; }
	float GetFountainSpread() const { return fountainSpread; }
	void  SetFountainSpread(float v) { fountainSpread = v; }
	float GetFountainDropSize() const { return fountainDropSize; }
	void  SetFountainDropSize(float v) { fountainDropSize = v; }
	int   GetFountainCount() const { return fountainCount; }
	void  SetFountainCount(int c) { fountainCount = c < 1 ? 1 : c; fountainInit = false; }
	// Droplet elongation: 0 = round dots, higher = longer streaks stretched along
	// each droplet's velocity (rain-like water).
	float GetFountainStretch() const { return fountainStretch; }
	void  SetFountainStretch(float v) { fountainStretch = v < 0.0f ? 0.0f : v; }
	// Advance + draw. RenderFountain is called by Game::Render right after
	// RenderWeather (same depth-tested, depth-write-off transparent slot).
	void RenderFountain(Game& game, const Renderer& renderer);

	// --- seasonal foliage ----------------------------------------------
	// Swaps the texture of every model whose material is `seasonal` (grass /
	// foliage) to a per-season variant: "<base>_spring/_autumn/_winter.png" (found
	// next to the base texture); SUMMER or a missing variant uses the base texture.
	// Authored via the ".scene" token `season <spring|summer|autumn|winter>`.
	enum class Season { Summer, Spring, Autumn, Winter };
	void SetSeason(Game& game, Season s);
	Season GetSeason() const { return season; }

	// --- exposure (linear workflow only: renderer.dat `linearLighting 1`) ---
	// A multiplier on the world's linear light before tonemapping; 0 = the
	// project's renderer.dat `exposure`. Fades over fadeSeconds when > 0.
	// Authored per scene with the ".scene" token `exposure <multiplier>` (reset
	// on every load), and from scripts with `scene3d exposure <v> [seconds]`.
	void SetExposure(float multiplier, float fadeSeconds = 0.0f);
	float GetExposure() const;   // this scene's value (0 = project default)
	// Bloom strength 0..1 (0 = none; < 0 = the project's renderer.dat `bloom`).
	// Authored with the ".scene" token `bloom <strength>`, reset on every load.
	void SetBloom(float strength);
	float GetBloom() const;      // this scene's value (< 0 = project default)
	// Image-based lighting from the sky (linear workflow): multipliers on the
	// sky's diffuse light and reflections; 0 = off, < 0 = renderer.dat `ibl`.
	// Authored with the ".scene" token `ibl <diffuse> [specular]`.
	void SetIBL(float diffuse, float specular);
	// Ambient occlusion (linear workflow): strength 0..1 (0 = off, < 0 = the
	// project's renderer.dat `ao`) and search radius in world units (<= 0 =
	// renderer.dat `aoRadius`). Authored with the ".scene" token
	// `ao <strength> [radius]`, reset on every load.
	void SetAmbientOcclusion(float strength, float radius = -1.0f);
	// Temporal anti-aliasing (linear workflow) blends each frame into the
	// image accumulated over earlier ones. After a hard camera cut that the
	// game makes itself (teleporting renderer.camera rather than going through
	// JumpToCamera, which already does this), call it so the old view isn't
	// blended into the first frames of the new one.
	void NotifyCameraCut();
	// Colour grading (linear workflow): a strip LUT PNG (N*N x N) applied to
	// the final image - "" = the project's renderer.dat `colorGrade`, "none" =
	// off. Cross-fades from the current look over fadeSeconds. Authored with
	// the ".scene" token `grade <png> [strength]`; scripts use
	// `scene3d grade <png|none|default> [strength] [seconds]`.
	void SetColorGrade(const std::string& lutPath, float strength = 1.0f, float fadeSeconds = 0.0f);
	// Depth of field (linear workflow): focus distance (world units in front
	// of the camera) and aperture - the blur, in 720p pixels, of what lies far
	// beyond the focus (0 = off); fades over fadeSeconds. The ".scene" token is
	// `dof <focus> <aperture>`; scripts use `scene3d dof ...`.
	void SetDepthOfField(float focusDistance, float aperture, float fadeSeconds = 0.0f);
	// The same, with the focus following a character's face every frame (a
	// cutscene close-up).
	void SetDepthOfFieldTarget(const std::string& characterName, float aperture, float fadeSeconds = 0.0f);
	// Game::Render: resolve a followed character's distance for this frame.
	void UpdateDepthOfField(const Renderer& renderer);
	// Volumetric fog and light shafts (linear workflow): density per world
	// unit (0 = none), fading over fadeSeconds. The ".scene" token is
	// `fog <density> [falloff] [r g b] [anisotropy] [noise]`; rain, snow and
	// storms bring their own when a scene sets none. Scripts: `scene3d fog`.
	void SetFog(float density, float fadeSeconds = 0.0f);
	// Game::Render: this frame has fog to draw (sets the weather's), and the
	// half-resolution march, lit like the scene (the composite onto the world
	// is render/VolumetricFog.h's CompositeFog, in its own pass).
	bool WantsVolumetricFog(const Renderer& renderer);
	void RenderVolumetricFog(Game& game, const Renderer& renderer);
	// What the sky looks like now (texture, time-of-day cross-fade, tint), for
	// the environment capture. False without an active scene with a sky.
	bool GetSkySource(Texture*& sky, Texture*& next, float& blend, glm::vec3& tint) const;

	// A global weather override (a debug/CLI flag or a story-wide storm): when set
	// to anything but None it is (re)applied after every scene load, overriding the
	// scene's own authored weather. Leave None so scenes use their authored weather.
	WeatherType forcedWeather = WeatherType::None;
	float forcedWeatherIntensity = 1.0f;

	// --- storm lightning & thunder -------------------------------------
	// Only active while weatherType == Storm. Sound effect played (after a
	// distance-based delay) when lightning strikes; set to "" to run the storm
	// silently. Games can also tune the strike cadence.
	std::string thunderSound = "assets/se/thunder.wav";
	int thunderChannel = 6;         // SDL_mixer channel (0..7) thunder plays on
	float lightningMinGap = 3.5f;   // seconds between strikes (at intensity 1)
	float lightningMaxGap = 12.0f;
	// Current full-screen flash brightness (0 = none). Exposed so a game could
	// react (e.g. briefly brighten its own 2D layer) if it wants.
	float GetLightningFlash() const { return flashIntensity; }

	// --- 3D scene editor support ---------------------------------------
	// Live object lists so the editor can ray-pick, display info, and move
	// them. Positions are the entities' own GetPosition(); after moving a
	// model call RecomputeModelBounds so its pick AABB tracks it.
	const std::vector<Scene3DModel*>& GetModels() const { return models; }
	const std::vector<Character3D*>& GetCharacters() const { return characters; }
	void RecomputeModelBounds(Scene3DModel* m) const;

	// Ray-pick the nearest model under a screen position (window pixels).
	// Returns nullptr on a miss. Used by both the editor and play-mode clue
	// collection.
	Scene3DModel* PickModel(Game& game, float sx, float sy) const;

	// A model type the editor's "Add" dropdown can instance.
	struct ModelDef { std::string obj; std::string tex; bool solid = false; };
	// The available model palette: every .obj in the scene's model folder,
	// paired with a texture (the scene's own pairing when known).
	std::vector<ModelDef> GetModelPalette() const;
	// Spawn a new model instance at pos and return it (nullptr if the scene
	// isn't loaded / the model can't be read). Added to the scene + entities.
	Scene3DModel* AddModelInstance(Game& game, const ModelDef& def, const glm::vec3& pos);
	// Remove a model / character by index (as given by GetModels/GetCharacters).
	bool RemoveModel(Game& game, int index);

	// --- runtime models: a game's own geometry drawn as scene models ---------
	// A mesh the game built itself (terrain, track...), drawn with the scene's
	// lighting, shadows and effects like any loaded model. The mesh is BORROWED:
	// the game keeps it alive while the model exists and frees it after
	// RemoveRuntimeModels. `texture` is an image path (loaded as a scene colour
	// texture), `material` a materials.txt name ("" = the default), the bounds
	// are the mesh's own (for culling and picking). Runtime models are never
	// written to a .scene file, and a scene load removes them like any model.
	// Returns nullptr when no scene is loaded.
	Scene3DModel* AddRuntimeModel(Game& game, Mesh* mesh, const std::string& texture,
		const std::string& material, const glm::vec3& localMin, const glm::vec3& localMax,
		const glm::vec3& pos = glm::vec3(0.0f));
	void RemoveRuntimeModels(Game& game);
	static bool IsRuntimeModel(const Scene3DModel* m);

	// --- a game's own moving geometry in the sun's shadow -----------------
	// For geometry a game draws itself each frame (a moving train): `draw` runs
	// inside each sun-shadow map render and calls DrawShadowMesh for each of its
	// meshes; `signature` returns a number that changes whenever that geometry
	// moves, so still frames keep their cached shadows. Empty = none.
	void SetShadowCasterHook(std::function<void()> draw, std::function<double()> signature);
	// Only inside the hook's draw: one mesh into the shadow map being drawn.
	void DrawShadowMesh(Mesh* mesh, const glm::mat4& model) const;

	// The sky panorama entity (nullptr without a sky), e.g. to drive its
	// cross-fade (nextTexture / blendToNext) from a game's time of day.
	Skybox* GetSkybox() const;
	bool RemoveCharacter(Game& game, int index);

	// Spawn a "layered" character (folder + body/head pose sprites) at runtime,
	// added to the scene + entities. Lets the character schedule place NPCs from
	// the world model instead of baking them into the .scene. Returns the entity.
	Character3D* AddCharacter(Game& game, const std::string& name, const std::string& folder,
		const std::string& bodyPose, const std::string& headExpr,
		const glm::vec3& pos, float height);
	// Remove a specific character entity (schedule despawn). false if not present.
	bool RemoveCharacter(Game& game, Character3D* ch);

	// Serialize the current scene (models, characters, lighting, cameras)
	// back to its .scene file, preserving the load order. Returns false on
	// write failure. Used by the editor's Save.
	bool SaveScene(Game& game);
	// Discard edits and reload the scene from disk (editor's Revert).
	bool Reload(Game& game);

	// --- editor camera management --------------------------------------
	// Add a named camera pose (appended to the order, so the first camera -
	// the startup view - is preserved) or update an existing one in place.
	void AddOrUpdateCamera(const std::string& name, const CamPose& pose);
	// The camera names in load order (order[0] is the startup camera).
	const std::vector<std::string>& CameraOrder() const { return cameraOrder; }
	// Look up a camera pose by name; false if unknown.
	bool GetCameraPose(const std::string& name, CamPose& out) const;
	// Make the named camera the startup one (move it to the front of the order).
	bool SetDefaultCamera(const std::string& name);
	// Remove a named camera. Returns false if unknown or it's the last one.
	bool RemoveCamera(const std::string& name);

	// --- named anchors (schedule stand-points; the "slot" .scene token) -----
	// Read access for the schedule / editor; the non-const overload lets the
	// editor add / move / rename anchors in place.
	const std::vector<SceneAnchor>& Anchors() const { return anchors; }
	std::vector<SceneAnchor>& Anchors() { return anchors; }
	// Look up an anchor's world position + facing by name. false if unknown.
	bool GetAnchor(const std::string& name, glm::vec3& outPos, float& outYaw) const;
	// Remove the anchor at the given index (as listed by Anchors()). false if OOB.
	bool RemoveAnchorAt(int index);

	// Names (no extension) of every data/scenes/*.scene, for the load dropdown.
	std::vector<std::string> GetSceneList() const;
	// Create (if new) and load an empty scene of the given name so the user can
	// start placing objects and Save straight to that name. Opens it if it
	// already exists. Returns false on a bad name / write failure.
	bool NewScene(Game& game, const std::string& name);

private:
	// Serialize the scene to a stream (shared by SaveScene and SerializeToString)
	// and parse it back (shared by Load and LoadFromString).
	void WriteScene(std::ostream& out) const;
	bool LoadFromStream(Game& game, std::istream& file,
		const std::string& sceneName, bool jumpCamera);

	std::vector<Scene3DModel*> models;
	std::vector<Character3D*> characters;
	// Optional equirectangular sky (the .scene "sky" line). Owned as an entity
	// (added to game.entities so it renders/updates); recreated per scene.
	Skybox* skybox = nullptr;
	std::string skyTexPath;          // kept for re-serialization
	float skyRadiusVal = 4000.0f;

	// Solid model footprints (world-space boxes) that characters are pushed
	// out of. minY/maxY are the world vertical extent (visual up = -Y, so
	// minY is the TOP). The 2-arg ResolveAgainstSolids overload ignores Y
	// (infinite columns, the historical behavior); the 4-arg overload only
	// blocks when the box overlaps the character's body span.
	struct SolidBox { float minX, maxX, minZ, maxZ, minY, maxY; };
	std::vector<SolidBox> solids;

	// Walkable-top footprints ("walk"-flagged models): XZ box + the world y
	// of the surface a character stands on (topY = box minY; up = -Y).
	struct GroundBox { float minX, maxX, minZ, maxZ, topY; };
	std::vector<GroundBox> grounds;

	// Current scene's lighting (all reset per Load)
	glm::vec3 ambientColor = glm::vec3(0.08f, 0.08f, 0.10f);
	SceneDirLight dirLight;
	std::vector<ScenePointLight> pointLights;
	std::vector<SceneSpotLight> spotLights;
	// Runtime focus spotlight (SpotlightCharacter), applied on top of the above.
	SceneSpotLight focusSpot;
	bool focusSpotOn = false;

	std::map<std::string, CamPose> cameras;
	std::vector<std::string> cameraOrder;  // first entry = default view
	std::vector<SceneAnchor> anchors;      // named stand-points ("slot" token)
	ShaderProgram* shader = nullptr;         // scene models
	ShaderProgram* billboardShader = nullptr;  // characters
	ShaderProgram* instancedShader = nullptr;  // scene models, instanced (dup props)
	ShaderProgram* edgeShader = nullptr;       // post-process depth-edge outline
	ShaderProgram* shadowDepthShader = nullptr;  // sun's-POV depth pass
	unsigned int shadowFBO = 0, shadowDepthTex = 0;

	// Unused: the Camera block moved to the renderer (Renderer::BindCameraBlock).
	// Kept so Scene3D's layout doesn't change for games built against it.
	unsigned int cameraUBO = 0;

	// Per-frame instance groups (>=2 duplicate opaque props each); group[0] leads.
	std::vector<std::vector<Scene3DModel*>> instanceGroups;
	void RebuildInstanceGroups();

	// GPU-driven models (Scene3DGpuDriven.cpp): this frame's instance list, and
	// one view's cull + draws for the colour passes (world, AO prepass) and the
	// depth passes (sun cascades, point-light cube faces). False = not active
	// this frame (the caller draws every model itself).
	void BuildGpuDrawList(const Renderer& renderer);
	bool DrawGpuColourView(const Renderer& renderer, unsigned int program, bool world);
	bool DrawGpuDepthView(const glm::mat4& viewProj, unsigned int program);
	int shadowMapSize = 2048;
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
	bool shadowActive = false;    // was the shadow map rendered this frame?
	double shadowSig = 0.0;       // cache key (sun dir + casters); moved -> re-render
	bool shadowEverRendered = false;
	void EnsureShadowMap();
	// Cascaded sun shadows (Scene3DShadows.cpp): the maps for this frame, and
	// one cascade's casters.
	void RenderShadowCascades(Game& game, const Renderer& renderer, const glm::vec3& L, double casterSig);
	void DrawShadowCasters(const Renderer& renderer, unsigned int program, const glm::vec3& center,
		float radius, const glm::vec3& L);

	// Omnidirectional (cube) shadow maps: up to kMaxPointShadows point lights cast
	// at once, each into its own cube. Depth is re-rendered only when the scene
	// changes (static caching), and each cube's depth pass culls out-of-range
	// geometry - so several lamp shadows in a mostly-static room are nearly free.
	// On a GL 4.x context: one cube-map ARRAY holds up to kMaxPointShadows cubes,
	// sampled with a runtime layer index (no fixed-sampler cap). On the 3.3/web
	// fallback: up to kMaxPointShadowsFallback separate cubes, constant-indexed.
	static const int kMaxPointShadows = 8;           // GL4 cube-array capacity
	static const int kMaxPointShadowsFallback = 4;   // 3.3/web separate-cube cap
	ShaderProgram* pointShadowShader = nullptr;
	unsigned int pointShadowFBO = 0;
	unsigned int pointShadowCubes[kMaxPointShadowsFallback] = { 0, 0, 0, 0 };
	unsigned int pointShadowArrayTex = 0;            // GL4: GL_TEXTURE_CUBE_MAP_ARRAY
	int pointShadowSize = 1024;
	int pointShadowCount = 0;                        // active casters this frame
	glm::vec3 pointShadowPositions[kMaxPointShadows];
	float pointShadowFars[kMaxPointShadows] = { 0 };
	bool pointShadowActive = false;
	double pointShadowSig = 0.0;                     // cache key (moved -> re-render)
	bool pointShadowEverRendered = false;
	void EnsurePointShadowMaps();
	Mesh* billboardQuad = nullptr;           // shared unit quad

	// Saved 2D camera state for restoring the VN view
	glm::vec3 saved2DCameraPos = glm::vec3(0, 0, 0);

	bool sceneEverLoaded = false;  // gates testAutoQuit until a scene has run

	// Camera glide state (blend 1.0 = settled on target)
	bool gliding = false;
	bool glideFromCaptured = false;
	std::string focusCharName;  // set by FocusCharacter; verified on glide end
	glm::vec3 focusTarget = glm::vec3(0);  // the look-at point (screen center)
	float focusHeadFrac = 0.97f;           // head-top billboard fraction, for the in-frame check
	float glideBlend = 1.0f;
	float glideSeconds = 0.9f;
	CamPose glideFrom;
	CamPose glideTo;

	void EnterPerspective(Game& game);
	void RestoreOrtho(Game& game);
	Character3D* FindCharacter(const std::string& name) const;
	float perspFovDeg = 60.0f;  // must match EnterPerspective's SetupPerspective
	Texture* ResolveTexture(Game& game, const std::string& folder,
		const std::string& part, const std::string& code,
		std::string* resolvedPath = nullptr);

	// Fill a character's figureTopFrac/figureBotFrac from the opaque alpha
	// bounds of its sprite(s) - the union of body + head for layered, or the
	// single sprite for combined. Leaves the defaults if the files can't be
	// read. bodyPath/headPath may be empty.
	void ComputeFigureBounds(Character3D* ch,
		const std::string& bodyPath, const std::string& headPath) const;

	// Parse an OBJ's vertices for a local AABB, transform by the model's
	// (translate * yaw * scale*scaleAxis), and return the world footprint
	// (XZ box + Y extent). Returns false if the OBJ can't be read.
	bool ComputeSolidBox(const std::string& objPath, const glm::vec3& pos,
		float yawDeg, float scale, const glm::vec3& scaleAxis, SolidBox& out) const;

	// Read an OBJ's vertex positions into a local-space AABB (for pick bounds).
	bool ReadObjLocalAABB(const std::string& objPath,
		glm::vec3& lo, glm::vec3& hi) const;

public:
	// Recompute the solid-footprint (and walkable-ground) lists from all
	// current models - call after any add/remove/flag edit (the editor's tile
	// mode does this directly).
	void RebuildSolids();
private:

	// --- seasonal foliage ----------------------------------------------
	Season season = Season::Summer;   // Summer = base textures (no season token)

	// --- weather particles ---------------------------------------------
	WeatherType weatherType = WeatherType::None;
	float weatherIntensity = 1.0f;
	static const int kMaxWeatherParticles = 8000;
	std::vector<glm::vec4> weatherParticles;   // xyz = world pos, w = random seed
	bool weatherInit = false;                  // volume seeded around the camera?
	ShaderProgram* weatherShader = nullptr;
	unsigned int weatherVAO = 0, weatherQuadVBO = 0, weatherInstVBO = 0;
	unsigned int snowTex = 0, rainTex = 0;     // procedurally generated sprites
	void EnsureWeatherResources();             // lazily build VAO/VBOs + textures
	void UpdateWeather(const glm::vec3& camPos, float dtSec);  // advance + wrap

	// Free-running clock (seconds) driving point-light strobes (flashHz).
	float flashClock = 0.0f;

	// --- storm lightning & thunder -------------------------------------
	float lightningTimer = 0.0f;    // seconds until the next strike
	float flashIntensity = 0.0f;    // current flash brightness, decays each frame
	float reflashTimer = -1.0f;     // pending secondary flicker (< 0 = none)
	float reflashMag = 0.0f;
	float thunderTimer = -1.0f;     // pending thunder sound-out (< 0 = none)
	bool  stormSeeded = false;      // first-strike delay armed for this storm?
	std::string flashShaderVert = "data/shaders/flash.vert";
	std::string flashShaderFrag = "data/shaders/flash.frag";
	ShaderProgram* flashShader = nullptr;
	unsigned int flashVAO = 0;                  // empty VAO for the full-screen tri
	void UpdateLightning(Game& game, float dtSec);   // strike timing, flash, thunder

	// --- fountain particle jet -----------------------------------------
	bool hasFountain = false;
	glm::vec3 fountainPos = glm::vec3(0.0f);   // nozzle (world); droplets spray up from here
	float fountainJetSpeed = 480.0f;           // initial up-speed (units/s; up = -Y)
	float fountainGravity = 900.0f;            // downward accel (units/s^2)
	float fountainFallDist = 175.0f;           // respawn once a droplet falls this far below the nozzle
	float fountainSpread = 60.0f;              // horizontal launch-velocity spread (the dome shape)
	float fountainDropSize = 9.0f;
	int   fountainCount = 340;
	float fountainStretch = 3.5f;              // 0 = dot; >0 = streak length (x dropSize) along velocity
	std::vector<glm::vec4> fountainParticles;  // xyz = world pos, w = seed (streamed to the instance buf)
	std::vector<glm::vec3> fountainVel;        // per-particle velocity (CPU sim + streamed for streaks)
	bool  fountainInit = false;
	unsigned int fountainVAO = 0, fountainInstVBO = 0, fountainVelVBO = 0;  // own VAO: quad + pos + velocity
	void EnsureFountainResources();            // build the fountain VAO (shares the weather quad/textures)
	void FountainRespawn(int i, bool stagger); // (re)launch particle i from the nozzle
	void UpdateFountain(float dtSec);          // integrate gravity + arcs, recycle
};

#endif
