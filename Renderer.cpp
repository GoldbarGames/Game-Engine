#include "Renderer.h"

int Renderer::SCALE = 2;

Renderer::Renderer()
{
	layersVisible["BACK"] = true;
	layersVisible["MIDDLE"] = true;
	layersVisible["OBJECT"] = true;
	layersVisible["COLLISION"] = true;
	layersVisible["FRONT"] = true;
}

Renderer::~Renderer()
{

}

int Renderer::GetScale()
{
	return SCALE;
}

void Renderer::SetScale(int s)
{
	SCALE = s;
}

bool Renderer::IsVisible(DrawingLayer layer)
{
	return layersVisible[GetDrawingLayerName(layer)];
}

void Renderer::ToggleVisibility(std::string layer)
{
	layersVisible[layer] = !layersVisible[layer];
}

int Renderer::RenderCopy(SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect)
{
	return SDL_RenderCopy(renderer, texture, srcrect, dstrect);
}

int Renderer::RenderCopyEx(SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect, 
	const double angle, const SDL_Point* center, const SDL_RendererFlip flip)
{
	return SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, angle, center, flip);
}

SDL_Texture* Renderer::CreateTextureFromSurface(SDL_Surface* surface)
{
	return SDL_CreateTextureFromSurface(renderer, surface);
}

void Renderer::CreateSDLRenderer(SDL_Window* window, bool vsync)
{
	//TODO: Destroying the renderer also destroys all textures rendered by it.
	// Somehow we must re-create all textures with the new renderer
	// Or, simply create two renderers at the start, and switch between them

	if (renderer != nullptr)
		SDL_DestroyRenderer(renderer);

	if (vsync)
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	else
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_RenderSetLogicalSize(renderer, screenWidth, screenHeight);
}