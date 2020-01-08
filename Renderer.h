#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include <unordered_map>
#include "globals.h"
#include "Shader.h"

class Renderer
{
private:
	std::unordered_map<std::string, bool> layersVisible;
	static int SCALE;
public:
	std::unordered_map<std::string, Shader*> shaders;
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