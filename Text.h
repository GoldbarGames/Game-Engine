#pragma once
#include "SDL.h"
#include <SDL_ttf.h>
#include <string>

using std::string;

class Renderer;

class Text
{
private:
	Renderer* renderer;
	TTF_Font* font;
public:
	std::string txt = "";
	SDL_Texture* textTexture = nullptr;
	SDL_Surface* textSurface = nullptr;
	SDL_Rect textWindowRect;
	SDL_Rect textTextureRect;
	Text(Renderer* newRenderer, TTF_Font* newFont);
	~Text();
	void SetText(string text);
	void Render(Renderer* renderer);
	void SetPosition(float x, float y);
	void SetFont(TTF_Font* newFont);
};

