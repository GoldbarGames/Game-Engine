#pragma once
#include "SDL.h"
#include <SDL_ttf.h>
#include <string>
#include "globals.h"

using std::string;

class Renderer;

class Text
{
private:
	Renderer* renderer;
	TTF_Font* font;
public:
	std::string id = ""; // this will always be english
	std::string txt = ""; // this might get translated
	SDL_Texture* textTexture = nullptr;
	SDL_Surface* textSurface = nullptr;
	SDL_Rect textWindowRect;
	SDL_Rect textTextureRect;
	Text(Renderer* newRenderer, TTF_Font* newFont);
	Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt);
	~Text();
	void SetText(string text, Color color = { 255, 255, 255, 255 });
	void SetTextWrapped(string text, Uint32 width);
	void Render(Renderer* renderer);
	void SetPosition(float x, float y);
	void SetFont(TTF_Font* newFont);
};

