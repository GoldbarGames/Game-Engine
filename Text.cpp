#include "Text.h"
#include "Renderer.h"
#include "Sprite.h"

int Text::GetTextWidth() 
{ 
	return textSprite->texture->GetWidth(); 
}
int Text::GetTextHeight() 
{ 
	return textSprite->texture->GetHeight(); 
}

//TODO: Refactor these constructors a little bit

Text::Text(Renderer* newRenderer, TTF_Font* newFont)
{
	renderer = newRenderer;
	font = newFont;
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
}

Text::Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt)
{
	renderer = newRenderer;
	font = newFont;
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
	SetText(txt);
}

Text::Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, Color color)
{
	renderer = newRenderer;
	font = newFont;
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
	SetText(txt, color);
}

Text::~Text()
{
	if (textSprite != nullptr)
		delete textSprite;
}

void Text::SetFont(TTF_Font* newFont)
{
	font = newFont;
}

void Text::SetText(string text, Color color)
{
	// empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (text == "")
		text = " ";

	if (textSprite != nullptr)
		delete textSprite;

	textColor = color;
	txt = text; // translate the text here
	id = text;

	SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a });
	
	Texture* textTexture = new Texture(text.c_str());
	textTexture->LoadTexture(textSurface);

	textSprite = new Sprite(textTexture, renderer->shaders["default"]);

	if (textSurface != nullptr)
		SDL_FreeSurface(textSurface);
}

void Text::SetTextWrapped(string text, Uint32 width)
{
	// empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (text == "")
		text = " ";

	if (textSprite != nullptr)
		delete textSprite;

	txt = text;
	SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), { 255, 255, 255, 255 }, width);
	
	Texture* textTexture = new Texture(text.c_str());
	textTexture->LoadTexture(textSurface);

	textSprite = new Sprite(textTexture, renderer->shaders["default"]);

	if (textSurface != nullptr)
		SDL_FreeSurface(textSurface);
}

void Text::Render(Renderer* renderer)
{
	if (textSprite != nullptr)
	{
		textSprite->Render(position, renderer);
	}
}

void Text::SetPosition(float x, float y)
{
	position.x = x;
	position.y = y;
}
