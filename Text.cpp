#include "Text.h"



Text::Text(SDL_Renderer* newRenderer, TTF_Font * newFont)
{
	renderer = newRenderer;
	font = newFont;
	textTextureRect.x = 0;
	textTextureRect.y = 0;
	SetPosition(0, 0);
}


Text::~Text()
{
}

void Text::SetFont(TTF_Font* newFont)
{
	font = newFont;
}

void Text::SetText(string text)
{
	textSurface = TTF_RenderText_Solid(font, text.c_str(), { 255, 255, 255, 255 });
	textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_QueryTexture(textTexture, NULL, NULL, &textTextureRect.w, &textTextureRect.h);
	textWindowRect.w = textTextureRect.w;
	textWindowRect.h = textTextureRect.h;
}

void Text::Render(SDL_Renderer* renderer)
{
	SDL_RenderCopy(renderer, textTexture, &textTextureRect, &textWindowRect);
}

void Text::SetPosition(float x, float y)
{
	textWindowRect.x = x;
	textWindowRect.y = y;
}
