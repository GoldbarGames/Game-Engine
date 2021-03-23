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
#include "Vector2.h"
#include "Timer.h"
#include "GUI.h"
#include "leak_check.h"

#include "Light.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

class Sprite;
class Game;
class HealthComponent;
class Renderable;

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
	void UseLight(ShaderProgram& shader) const;

	Color overlayColor{ 0, 0, 0, 0 };
	Color targetColor{ 0, 0, 0, 0 };
	Color startColor{ 0, 0, 0, 0 };
	bool changingOverlayColor = false;
	Timer timerOverlayColor;
	Uint32 overlayStartTime = 0;
	Uint32 overlayEndTime = 0;

	mutable Vector2 debugScale = Vector2(1, 1);
	mutable Vector2 overlayScale = Vector2(1, 1);

	void RenderDebugRect(const SDL_Rect& targetRect, const Vector2& targetScale, Color color = { 255, 255, 255, 255 }) const;
	void RenderDebugRect(const SDL_Rect& targetRect, const Vector2& targetScale, const Vector2& targetPivot, Color color = { 255, 255, 255, 255 }) const;
	Vector2 CalculateScale(const Sprite& sourceSprite, int targetWidth, int targetHeight, const Vector2& targetScale) const;
	Vector2 screenScale = Vector2(1, 1);

	ShaderProgram* GetShader(int key);
	mutable std::unordered_map<int, ShaderProgram*> shaders;
	
	void LerpColor(float& color, float target, const float& speed);
	void FadeOverlay(const int screenWidth, const int screenHeight) const;
	void ToggleVisibility(DrawingLayer layer);
	bool IsVisible(DrawingLayer layer) const;

	void CreateShader(const int shaderName, const char* vertexFilePath, const char* fragmentFilePath);
	void CreateShaders();

	static ShaderProgram* GetTextShader();

	void Init(Game* g);
	Renderer();
	~Renderer();
};

#endif