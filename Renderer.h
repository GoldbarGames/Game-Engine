#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include <unordered_map>
#include "globals.h"
#include "Shader.h"
#include "Camera.h"

class Renderer
{
private:
	std::unordered_map<std::string, bool> layersVisible;
public:
	Camera camera;

	float now = 0;

	std::unordered_map<std::string, ShaderProgram*> shaders;

	void ToggleVisibility(std::string layer);
	bool IsVisible(DrawingLayer layer);

	void CreateShader(const char* shaderName, const char* vertexFilePath, const char* fragmentFilePath);

	Renderer();
	~Renderer();
};