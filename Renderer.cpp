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