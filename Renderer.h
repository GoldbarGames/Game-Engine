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
	GLuint uniformProjection = 0;
	GLuint uniformModel = 0;
	GLuint uniformView = 0;
	GLuint uniformMultiplyColor = 0;
	GLuint uniformViewTexture = 0;
	GLuint uniformOffsetTexture = 0;

	float now = 0;

	std::unordered_map<std::string, ShaderProgram*> shaders;

	void ToggleVisibility(std::string layer);
	bool IsVisible(DrawingLayer layer);

	Renderer();
	~Renderer();
};