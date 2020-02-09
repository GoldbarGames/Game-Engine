#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include <unordered_map>
#include "globals.h"
#include "Shader.h"
#include "Camera.h"
#include "Vector2.h"

class Sprite;

class Renderer
{
private:
	std::unordered_map<std::string, bool> layersVisible;
public:
	Camera camera;
	Sprite* debugSprite = nullptr;
	int drawCallsPerFrame = 0;

	float now = 0;

	Vector2 CalculateScale(Sprite* sourceSprite, Sprite* targetSprite);

	std::unordered_map<std::string, ShaderProgram*> shaders;

	void ToggleVisibility(std::string layer);
	bool IsVisible(DrawingLayer layer);

	void CreateShader(const char* shaderName, const char* vertexFilePath, const char* fragmentFilePath);

	Renderer();
	~Renderer();
};