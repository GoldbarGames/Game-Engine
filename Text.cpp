#include "Text.h"
#include "Renderer.h"
#include "Sprite.h"

int Text::GetTextWidth() 
{ 
	if (textSprite != nullptr)
		return textSprite->texture->GetWidth();
	else
		return 1;
}

int Text::GetTextHeight() 
{ 
	if (textSprite != nullptr)
		return textSprite->texture->GetHeight();
	else
		return 1;
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

Text::Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, 
	bool relPos, bool relScale)
{
	renderer = newRenderer;
	font = newFont;
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
	SetText(txt);

	if (textSprite != nullptr)
	{
		textSprite->keepPositionRelativeToCamera = relPos;
		textSprite->keepScaleRelativeToCamera = relScale;
	}
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
	{
		delete_it(textSprite->texture);
		delete_it(textSprite);
	}
}

void Text::SetFont(TTF_Font* newFont)
{
	font = newFont;
}

//TODO: Maybe modify this or make another function to pass in a shader?
void Text::SetText(string text, Color color, Uint32 wrapWidth)
{
	bool keepScaleRelative = false;
	bool renderRelative = false;

	if (textSprite != nullptr)
	{
		renderRelative = textSprite->keepPositionRelativeToCamera;
		keepScaleRelative = textSprite->keepScaleRelativeToCamera;
		delete_it(textSprite->texture);
		delete_it(textSprite);
	}

	textColor = color;
	txt = text; // translate the text here
	id = text;

    // empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (txt == "")
		txt = " ";


	SDL_Surface* textSurface = nullptr;
	SDL_Color textColor = { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a };
	
	if (wrapWidth > 0)
	{
		textSurface = TTF_RenderText_Blended_Wrapped(font, txt.c_str(), textColor, wrapWidth);
	}
	else
	{
		textSurface = TTF_RenderText_Blended(font, txt.c_str(), textColor);
	}

	if (textSurface != nullptr)
	{
		Texture* textTexture = new Texture(txt.c_str());
		textTexture->LoadTexture(textSurface);

		textSprite = new Sprite(textTexture, renderer->shaders["gui"]);
		textSprite->keepScaleRelativeToCamera = keepScaleRelative;
		textSprite->keepPositionRelativeToCamera = renderRelative;

		if (textSurface != nullptr)
			SDL_FreeSurface(textSurface);
	}
}

void Text::Render(Renderer* renderer)
{
	if (textSprite != nullptr)
	{
		textSprite->Render(position, renderer);
	}
}

void Text::Render(Renderer* renderer, Vector2 offset)
{
	if (textSprite != nullptr)
	{
		textSprite->Render(position + offset, renderer);
	}
}

void Text::SetPosition(float x, float y)
{
	position.x = x;
	position.y = y;
}

void Text::SetPosition(int x, int y)
{
	position.x = (float)x;
	position.y = (float)y;
}
