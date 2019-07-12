#pragma once
#include "SDL.h"
#include <SDL_ttf.h>
#include <string>

using std::string;

class Text
{
private:
	SDL_Renderer* renderer;
	TTF_Font* font;
public:
	SDL_Texture* textTexture = nullptr;
	SDL_Surface* textSurface = nullptr;
	SDL_Rect textWindowRect;
	SDL_Rect textTextureRect;
	Text(SDL_Renderer* newRenderer, TTF_Font* newFont);
	~Text();
	void SetText(string text);
	void Render(SDL_Renderer* renderer);
	void SetPosition(float x, float y);
	void SetFont(TTF_Font* newFont);
};

