#ifndef RENDERER_H
#define RENDERER_H
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <unordered_map>
#include "globals.h"
#include "Shader.h"
#include "Camera.h"
#include "Timer.h"
#include "GUI.h"
#include "leak_check.h"

#include "Light.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

class Sprite;
class Game;
class HealthComponent;
class Renderable;
class Texture;
class Mesh;

// One view of the 3D world in split screen (Renderer::SetViews): a camera and
// the part of the window it fills, as fractions of the window measured from
// its top-left corner (0..1), so it follows resolution changes.
struct RenderView
{
	Camera* camera = nullptr;     // the game's; its projection is rebuilt for the view's shape each frame
	float x = 0.0f, y = 0.0f;     // top-left corner
	float width = 1.0f, height = 1.0f;
};

class KINJO_API Renderer
{
private:
	static ShaderProgram* textShader;
	mutable std::unordered_map<DrawingLayer, bool> layersVisible;
public:
	static ShaderProgram* tileShader;
	Camera camera;
	Camera guiCamera;

	Light* light = nullptr;

	bool hotReloadShaders = true;
	
	PointLight* pointLights[MAX_POINT_LIGHTS];
	SpotLight* spotLights[MAX_SPOT_LIGHTS];
	mutable unsigned int pointLightCount = 0;
	mutable unsigned int spotLightCount = 0;

	Sprite* debugSprite = nullptr;
	Sprite* overlaySprite = nullptr;

	mutable int drawCallsPerFrame = 0;
	float now = 0;
	Game* game;

	void Update();
	void UseLight(const ShaderProgram& shader) const;

	Color overlayColor{ 0, 0, 0, 0 };
	Color targetColor{ 0, 0, 0, 0 };
	Color startColor{ 0, 0, 0, 0 };
	bool changingOverlayColor = false;
	Timer timerOverlayColor;
	Uint32 overlayStartTime = 0;
	Uint32 overlayEndTime = 0;

	mutable glm::vec2 debugScale = glm::vec2(1, 1);
	mutable glm::vec2 overlayScale = glm::vec2(1, 1);

	void RenderDebugRect(const SDL_Rect& targetRect, const glm::vec2& targetScale, Color color = { 255, 255, 255, 255 }) const;
	void RenderDebugRect(const SDL_Rect& targetRect, const glm::vec2& targetScale, const glm::vec2& targetPivot, Color color = { 255, 255, 255, 255 }) const;
	glm::vec2 CalculateScale(const Sprite& sourceSprite, int targetWidth, int targetHeight, const glm::vec2& targetScale) const;
	glm::vec2 screenScale = glm::vec2(1, 1);

	ShaderProgram* GetShader(int key) const;
	mutable std::unordered_map<int, ShaderProgram*> shaders;

	Timer reloadTimer;
	std::vector<std::string> shaderList;
	std::string shaderFolder = "data/shaders/";

	std::unordered_map<std::string, std::filesystem::file_time_type> lastModified;

	void HotReload();
	
	void LerpColor(float& color, float target, const float& speed);
	void FadeOverlay(const int screenWidth, const int screenHeight) const;
	void ToggleVisibility(DrawingLayer layer);
	bool IsVisible(DrawingLayer layer) const;

	void CreateShader(const int shaderName, const char* vertexFilePath, const char* fragmentFilePath, bool fromString = false);
	void CreateShaders();

	static ShaderProgram* GetTextShader();

	int instanceAmount = 0;
	void ConfigureInstanceArray(unsigned int amount=100000);
	glm::mat4* modelMatrices = nullptr;

	// Instanced batch rendering (mutable for const-correct batching in Sprite::Render)
	static const int MAX_BATCH_SIZE = 10000;
	unsigned int instanceVBO = 0;
	Mesh* batchMesh = nullptr;
	mutable std::vector<glm::mat4> batchMatrices;
	mutable std::vector<glm::vec4> batchTexData;  // xy = texOffset, zw = texFrame
	mutable std::vector<glm::vec4> batchColors;
	mutable Texture* currentBatchTexture = nullptr;
	mutable ShaderProgram* currentBatchShader = nullptr;
	ShaderProgram* instancedShader = nullptr;  // Dedicated shader for instanced batch rendering
	bool batchingEnabled = false;  // Set to true after InitBatchRendering()

	void InitBatchRendering();
	void BeginBatch(Texture* texture, ShaderProgram* shader) const;
	void AddToBatch(const glm::mat4& model, const glm::vec2& texOffset, const glm::vec2& texFrame, const Color& color) const;
	void FlushBatch() const;
	void EndBatch() const;

	// Is the instanced-draw machinery (shader + quad + buffer) usable? Text uses
	// this to decide whether to batch a glyph run or fall back to per-glyph draws.
	bool GlyphBatchReady() const;
	// Draw a run of glyphs that all share one font-atlas texture in ONE instanced
	// call, using the GUI camera. Each entry: model matrix (from the glyph's own
	// CalculateModel), texData = vec4(uvOffset.xy, uvSize.xy), and color. This is a
	// dedicated path (NOT the sprite auto-batch), so normal sprite rendering is
	// unaffected. Used by Text::Render for ASCII-atlas rich text.
	void DrawGlyphBatch(Texture* atlas, const std::vector<glm::mat4>& models,
		const std::vector<glm::vec4>& texData, const std::vector<glm::vec4>& colors) const;

	// ---- Overlay drawing (use these instead of raw GL in game code) ----
	// Flat-colored shapes for menus, HUDs and editor gizmos. Each call draws
	// immediately, so it layers correctly with Text/Sprite renders issued around
	// it. Alpha-blended, depth test off; GL state is left as it was found. The
	// shader is built into the engine (no data/shaders files needed).
	//
	// GUI space: GUI units (designWidth * Camera::MULTIPLIER wide), origin top-left.
	void DrawRect(float x, float y, float w, float h, const glm::vec4& color) const;
	// World space through the game camera, drawn on top of the scene. `segments`
	// holds pairs of endpoints (GL_LINES style), so its size must be even.
	void DrawLines3D(const std::vector<glm::vec3>& segments, const glm::vec4& color) const;

	// ---- Camera uniform block ("Camera", binding 0; shaders/camera.glsl) ----
	// Every draw path binds the camera it draws with right before drawing; no
	// code may assume the block is still bound from an earlier draw. Buffers
	// are cached by content, so alternating between cameras (world sprites,
	// GUI text, Scene3D) rebinds instead of re-uploading.
	void BindCameraBlock(const glm::mat4& view, const glm::mat4& projection) const;
	// Shorthand for the world camera: camera's view + perspective/ortho projection.
	void BindWorldCameraBlock() const;

	// Split screen (since 2026-10-05): the 3D world is drawn once per view,
	// each through its own camera into its own part of the window; the
	// cutscene layer, GUI and menus then go over the whole window once. Up to
	// 4 views (more are ignored). Each frame a view camera's projection is
	// rebuilt at its view's aspect from its fov, nearPlane and farPlane, and
	// renderer.camera stays the main camera (picking, the editor, JumpToCamera).
	// Views of one size share the engine's temporary targets; views of
	// different sizes work, but reallocate them every frame. Count 0, or
	// ClearViews(), goes back to the one full-window renderer.camera.
	void SetViews(const RenderView* views, int count);
	void ClearViews();
	int ViewCount() const;
	const RenderView* GetViews() const;

	// Renderer settings from game code (since 2026-10-09; a player's graphics
	// level): a `key value` as renderer.dat writes it, kept over the file's value
	// until cleared, so the project's file stays as authored. ApplyRenderSettings
	// makes the changes take effect, mid-game too, as the 3D editor's PROJECT
	// tab does. linearLighting and gpuDriven need a restart, so they are refused
	// (false, with a log line). render/RenderSettings.cpp.
	bool SetRenderSetting(const std::string& key, const std::string& value);
	void ClearRenderSettings();
	void ApplyRenderSettings();

	void Init(Game* g);
	void SetDepthTestEnabled(bool enabled) const;
	void SetDepthBias(float factor, float units) const;
	void ClearDepthBias() const;
	Renderer();
	~Renderer();

private:
	// Overlay GL objects live in RendererOverlay.cpp (file-local), not as members,
	// so adding the overlay API did not change Renderer's layout.
	static void ReleaseOverlayResources();
	// Same for every uniform-block buffer (RendererBlocks.cpp).
	static void ReleaseUniformBlocks();
	// UseLight for programs with the SpriteLights block: fill and bind it.
	// False if the program predates the block (RendererBlocks.cpp).
	bool BindSpriteLightsBlock(const ShaderProgram& shader) const;
};

#endif