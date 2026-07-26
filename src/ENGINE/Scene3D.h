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
	bool solid = false;
	// Game-agnostic interaction tag: an arbitrary label a game can attach to a
	// model (a clue id, a door name, a pickup type, ...). Authored via the
	// .scene "model ... tag <VALUE>" field or the editor's TAG button. The
	// engine only stores and serializes it; games decide what it means.
	std::string interactionTag;
	bool tagTriggered = false;    // runtime scratch: game flag (e.g. already used)
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
	// Full-screen depth-edge outline shader (created on first scene load), run
	// by Game's composite pass. Public getter.
	ShaderProgram* EdgeShader() const { return edgeShader; }

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
	// Names of the scene's point lights, in order (for the editor's cycle button).
	std::vector<std::string> PointLightNames() const;

	// Hard-cut the camera to a named pose. Returns false if unknown.
	bool JumpToCamera(Game& game, const std::string& camName);

	// Start a smooth glide (smoothstep, shortest-path yaw) to a named pose.
	bool GlideToCamera(const std::string& camName, float seconds);

	// Smoothly frame a named character. closeup=false frames the whole
	// figure centered; closeup=true frames an upper-body dialogue shot
	// (waist to just above the head), dollying the camera in/out along its
	// view axis to fit - the framing adapts to each character's proportions.
	// distanceOverride (> 0) forces a fixed camera distance instead.
	bool FocusCharacter(Game& game, const std::string& charName,
		float seconds, bool closeup = false, float distanceOverride = 0.0f);

	// Push a floor point (x,z; y ignored) out of every solid model's
	// footprint until it clears them all, treating the point as a circle of
	// the given radius. Reusable for future character movement. No-op if the
	// point is already clear or there are no solids.
	void ResolveAgainstSolids(glm::vec3& pos, float radius) const;

	// Advance an in-progress glide; call every frame while active.
	void Update(Game& game);

	// Upload the current scene lighting (ambient + directional + point +
	// spot) to a bound shader program. Both the model and billboard
	// shaders share the same light uniform names.
	void ApplyLighting(unsigned int shaderID) const;

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

	// Runtime light control (by the name given in the .scene file). All
	// return false if no light of that name exists.
	bool SetLightOn(const std::string& name, bool on);
	bool SetLightIntensity(const std::string& name, float intensity);
	bool FadeLightIntensity(const std::string& name, float target, float seconds);
	bool SetLightColor(const std::string& name, const glm::vec3& color);
	bool SetLightPosition(const std::string& name, const glm::vec3& pos);

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
	// Cross-fade the sky between two panoramas: blend 0 = fully fromPath,
	// 1 = fully toPath. Textures come from the sprite cache (cheap per frame).
	// Pass an empty toPath (or blend 0) for a single static panorama.
	void SetSkyCrossfade(Game& game, const std::string& fromPath,
		const std::string& toPath, float blend);

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
	bool RemoveCharacter(Game& game, int index);

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

	// Solid model footprints (world-space XZ boxes) that characters are
	// pushed out of. Treated as infinite vertical columns (floor furniture).
	struct SolidBox { float minX, maxX, minZ, maxZ; };
	std::vector<SolidBox> solids;

	// Current scene's lighting (all reset per Load)
	glm::vec3 ambientColor = glm::vec3(0.08f, 0.08f, 0.10f);
	SceneDirLight dirLight;
	std::vector<ScenePointLight> pointLights;
	std::vector<SceneSpotLight> spotLights;

	std::map<std::string, CamPose> cameras;
	std::vector<std::string> cameraOrder;  // first entry = default view
	ShaderProgram* shader = nullptr;         // scene models
	ShaderProgram* billboardShader = nullptr;  // characters
	ShaderProgram* instancedShader = nullptr;  // scene models, instanced (dup props)
	ShaderProgram* edgeShader = nullptr;       // post-process depth-edge outline
	ShaderProgram* shadowDepthShader = nullptr;  // sun's-POV depth pass
	unsigned int shadowFBO = 0, shadowDepthTex = 0;

	// Shared camera matrices (std140 UBO, binding point 0): view + projection,
	// uploaded once per frame instead of per draw. Core GL 3.1 / ES 3.0.
	unsigned int cameraUBO = 0;
	void EnsureCameraUBO();
	// Link a program's "Camera" uniform block to binding point 0 (no-op if absent).
	void BindCameraBlock(ShaderProgram* program) const;

	// Per-frame instance groups (>=2 duplicate opaque props each); group[0] leads.
	std::vector<std::vector<Scene3DModel*>> instanceGroups;
	void RebuildInstanceGroups();
	int shadowMapSize = 2048;
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
	bool shadowActive = false;    // was the shadow map rendered this frame?
	void EnsureShadowMap();

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
	void GlideToPose(const CamPose& pose, float seconds);  // arbitrary target
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
	// (translate * yaw * scale), and return the world XZ footprint. Returns
	// false if the OBJ can't be read.
	bool ComputeSolidBox(const std::string& objPath, const glm::vec3& pos,
		float yawDeg, float scale, SolidBox& out) const;

	// Read an OBJ's vertex positions into a local-space AABB (for pick bounds).
	bool ReadObjLocalAABB(const std::string& objPath,
		glm::vec3& lo, glm::vec3& hi) const;

	// Recompute the solid-footprint list from all current solid models (after
	// an add/remove edit).
	void RebuildSolids();
};

#endif
