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
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

class Sprite;
class Game;
class HealthComponent;
class Renderable;
class Texture;
class Mesh;

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

	// Instanced batch rendering
	static const int MAX_BATCH_SIZE = 10000;
	GLuint instanceVBO = 0;
	Mesh* batchMesh = nullptr;
	std::vector<glm::mat4> batchMatrices;
	std::vector<glm::vec4> batchTexData;  // xy = texOffset, zw = texFrame
	std::vector<glm::vec4> batchColors;
	Texture* currentBatchTexture = nullptr;
	ShaderProgram* currentBatchShader = nullptr;
	bool batchingEnabled = true;

	void InitBatchRendering();
	void BeginBatch(Texture* texture, ShaderProgram* shader);
	void AddToBatch(const glm::mat4& model, const glm::vec2& texOffset, const glm::vec2& texFrame, const Color& color);
	void FlushBatch();
	void EndBatch();

	void Init(Game* g);
	Renderer();
	~Renderer();
};

#endif