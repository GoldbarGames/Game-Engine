#include "Text.h"
#include "Renderer.h"

//TODO: Refactor these constructors a little bit

Text::Text(Renderer* newRenderer, TTF_Font* newFont)
{
	renderer = newRenderer;
	font = newFont;
	textTextureRect.x = 0;
	textTextureRect.y = 0;
	SetPosition(0, 0);
}

Text::Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt)
{
	renderer = newRenderer;
	font = newFont;
	textTextureRect.x = 0;
	textTextureRect.y = 0;
	SetPosition(0, 0);
	SetText(txt);
}

Text::Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, Color color)
{
	renderer = newRenderer;
	font = newFont;
	textTextureRect.x = 0;
	textTextureRect.y = 0;
	SetPosition(0, 0);
	SetText(txt, color);
}

Text::~Text()
{

}

void Text::SetFont(TTF_Font* newFont)
{
	font = newFont;
}

void Text::SetText(string text, Color color)
{
	if (textSurface != nullptr)
		SDL_FreeSurface(textSurface);

	if (textTexture != nullptr)
		SDL_DestroyTexture(textTexture);

	textColor = color;
	txt = text; // translate the text here
	id = text;
	textSurface = TTF_RenderText_Solid(font, text.c_str(), { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a });
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

void Text::Render(Renderer* renderer, GLuint uniformModel)
{
	renderer->RenderCopy(textTexture, &textTextureRect, &textWindowRect);
}

void Text::SetPosition(float x, float y)
{
	textWindowRect.x = x;
	textWindowRect.y = y;
}
