#ifndef RENDERER_H
#define RENDERER_H
#pragma once

#include "SDL.h"
#include "SDL_image.h"
#include <vector>
#include <unordered_map>
#include "globals.h"
#include "Shader.h"
#include "Camera.h"
#include "Vector2.h"
#include "Timer.h"
#include "GUI.h"

class Sprite;
class Game;
class HealthComponent;
class Renderable;

class Renderer
{
private:
	static ShaderProgram* textShader;
	mutable std::unordered_map<DrawingLayer, bool> layersVisible;
public:
	Camera camera;
	Camera guiCamera;

	Sprite* debugSprite = nullptr;
	Sprite* overlaySprite = nullptr;

	mutable int drawCallsPerFrame = 0;
	float now = 0;
	Game* game;

	void Update();

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

	mutable std::unordered_map<ShaderName, ShaderProgram*> shaders;
	
	void LerpColor(float& color, float target, const float& speed);
	void FadeOverlay(const int screenWidth, const int screenHeight) const;
	void ToggleVisibility(DrawingLayer layer);
	bool IsVisible(DrawingLayer layer) const;

	void CreateShader(const ShaderName shaderName, const char* vertexFilePath, const char* fragmentFilePath);
	void CreateShaders();

	static ShaderProgram* GetTextShader();

	void Init(Game* g);
	Renderer();
	~Renderer();
};

#endif