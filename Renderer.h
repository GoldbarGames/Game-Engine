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
	static int SCALE;
public:
	Camera camera;
	GLuint uniformProjection = 0;
	GLuint uniformModel = 0;
	GLuint uniformView = 0;
	GLuint uniformMultiplyColor = 0;
	GLuint uniformViewTexture = 0;
	float now = 0;

	std::unordered_map<std::string, ShaderProgram*> shaders;
	static int GetScale();
	static void SetScale(int s);
	SDL_Renderer * renderer;
	int RenderCopy(SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect);

	int RenderCopyEx(SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect,
		const double angle, const SDL_Point* center, const SDL_RendererFlip flip);

	SDL_Texture* CreateTextureFromSurface(SDL_Surface* surface);

	void ToggleVisibility(std::string layer);
	bool IsVisible(DrawingLayer layer);

	void CreateSDLRenderer(SDL_Window* window, bool vsync);

	Renderer();
	~Renderer();
};