#include "Text.h"
#include "Renderer.h"
#include "Sprite.h"

int Text::GetTextWidth() 
{ 
	if (currentSprite != nullptr)
		return currentSprite->texture->GetWidth();
	else
		return 1;
}

int Text::GetTextHeight() 
{ 
	if (currentSprite != nullptr)
		return currentSprite->texture->GetHeight();
	else
		return 1;
}

//TODO: Refactor these constructors a little bit
Text::Text(Renderer* newRenderer, TTF_Font* newFont) : Entity(Vector2(0,0))
{
	renderer = newRenderer;
	font = newFont;
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
}

Text::Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, 
	bool relPos, bool relScale) : Entity(Vector2(0, 0))
{
	renderer = newRenderer;
	font = newFont;
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
	SetText(txt);

	if (currentSprite != nullptr)
	{
		currentSprite->keepPositionRelativeToCamera = relPos;
		currentSprite->keepScaleRelativeToCamera = relScale;
	}
}

Text::Text(Renderer* newRenderer, TTF_Font* newFont, std::string txt, Color color) : Entity(Vector2(0, 0))
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
	if (currentSprite != nullptr)
	{
		delete_it(currentSprite->texture);
		delete_it(currentSprite);
	}
}

void Text::SetFont(TTF_Font* newFont)
{
	font = newFont;
}

//TODO: Maybe modify this or make another function to pass in a shader?
void Text::SetText(string text, Color color, Uint32 wrapWidth)
{
	// don't do anything if it would result in the same thing
	if (txt == text && textColor == color)
		return;

	bool keepScaleRelative = false;
	bool renderRelative = false;

	if (currentSprite != nullptr)
	{
		renderRelative = currentSprite->keepPositionRelativeToCamera;
		keepScaleRelative = currentSprite->keepScaleRelativeToCamera;
		delete_it(currentSprite->texture);
		delete_it(currentSprite);
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

		currentSprite = new Sprite(textTexture, renderer->shaders[ShaderName::GUI]);
		currentSprite->keepScaleRelativeToCamera = keepScaleRelative;
		currentSprite->keepPositionRelativeToCamera = renderRelative;

		if (textSurface != nullptr)
			SDL_FreeSurface(textSurface);
	}
}

void Text::Render(Renderer* renderer)
{
	if (currentSprite != nullptr)
	{
		currentSprite->Render(position, renderer);
	}
}

void Text::Render(Renderer* renderer, Vector2 offset)
{
	if (currentSprite != nullptr)
	{
		currentSprite->Render(position + offset, renderer);
	}
}

void Text::SetPosition(const float x, const float y)
{
	position.x = x;
	position.y = y;
}

void Text::SetPosition(const int x, const int y)
{
	position.x = (float)x;
	position.y = (float)y;
}
