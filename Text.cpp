#include "Text.h"
#include "Renderer.h"


Text::Text(Renderer* newRenderer, TTF_Font * newFont)
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
	if (textSurface != nullptr)
		SDL_FreeSurface(textSurface);

	if (textTexture != nullptr)
		SDL_DestroyTexture(textTexture);

	txt = text;
	textSurface = TTF_RenderText_Solid(font, text.c_str(), { 255, 255, 255, 255 });
	textTexture = renderer->CreateTextureFromSurface(textSurface);
	SDL_QueryTexture(textTexture, NULL, NULL, &textTextureRect.w, &textTextureRect.h);
	textWindowRect.w = textTextureRect.w;
	textWindowRect.h = textTextureRect.h;
}

void Text::SetTextWrapped(string text, Uint32 width)
{
	if (textSurface != nullptr)
		SDL_FreeSurface(textSurface);

	if (textTexture != nullptr)
		SDL_DestroyTexture(textTexture);

	txt = text;
	textSurface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), { 255, 255, 255, 255 }, width);
	textTexture = renderer->CreateTextureFromSurface(textSurface);
	SDL_QueryTexture(textTexture, NULL, NULL, &textTextureRect.w, &textTextureRect.h);
	textWindowRect.w = textTextureRect.w;
	textWindowRect.h = textTextureRect.h;
}

void Text::Render(Renderer* renderer)
{
	renderer->RenderCopy(textTexture, &textTextureRect, &textWindowRect);
}

void Text::SetPosition(float x, float y)
{
	textWindowRect.x = x;
	textWindowRect.y = y;
}
