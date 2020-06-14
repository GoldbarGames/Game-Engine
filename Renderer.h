#ifndef RENDERER_H
#define RENDERER_H
#pragma once

#include "SDL.h"
#include "SDL_image.h"
#include <unordered_map>
#include "globals.h"
#include "Shader.h"
#include "Camera.h"
#include "Vector2.h"
#include "Timer.h"

class Sprite;


class Renderer
{
private:
	std::unordered_map<DrawingLayer, bool> layersVisible;
	void UpdateOverlayColor(uint8_t& color, const int& target);
public:
	Camera camera;
	Camera guiCamera;
	Sprite* debugSprite = nullptr;
	Sprite* overlaySprite = nullptr;
	int drawCallsPerFrame = 0;

	float now = 0;

	void Update();

	Color overlayColor{ 0, 0, 0, 0 };
	Color targetColor{ 0, 0, 0, 0 };
	bool changingOverlayColor = false;
	Timer timerOverlayColor;

	Vector2 CalculateScale(Sprite* sourceSprite, Sprite* targetSprite);
	Vector2 CalculateScale(Sprite* sourceSprite, int targetWidth, int targetHeight, const Vector2& targetScale);
	Vector2 screenScale = Vector2(1, 1);

	std::unordered_map<ShaderName, ShaderProgram*> shaders;
	
	void FadeOverlay(const int screenWidth, const int screenHeight);
	void ToggleVisibility(DrawingLayer layer);
	bool IsVisible(DrawingLayer layer);

	void CreateShader(const ShaderName shaderName, const char* vertexFilePath, const char* fragmentFilePath);

	Renderer();
	~Renderer();
};

#endif