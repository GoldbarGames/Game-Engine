#include "Text.h"
#include "Renderer.h"
#include "Sprite.h"

int Text::GetTextWidth() 
{ 
	int width = 1;
	for (int i = 0; i < glyphs.size(); i++)
	{
		if (glyphs[i]->sprite != nullptr)
			width += glyphs[i]->sprite->texture->GetWidth();
	}

	return width;

	/*
	if (currentSprite != nullptr)
		return currentSprite->texture->GetWidth();
	else
		return 1;
		*/
}

int Text::GetTextHeight() 
{
	int width = 0;
	int height = 1;

	for (int i = 0; i < glyphs.size(); i++)
	{
		if (glyphs[i]->sprite != nullptr)
		{
			width += glyphs[i]->sprite->texture->GetWidth();
			if (width > wrapWidth)
			{
				width = 0;
				height += glyphs[i]->sprite->texture->GetHeight();				
			}
			else if (wrapWidth == 0)
			{
				return glyphs[0]->sprite->texture->GetHeight();
			}				
		}			
	}

	if (glyphs.size() > 0)
	{
		if (glyphs[0]->sprite != nullptr && glyphs[0]->sprite->texture != nullptr)
		{
			return glyphs[0]->sprite->texture->GetHeight();
		}
	}

	return height;

	/*
	if (currentSprite != nullptr)
		return currentSprite->texture->GetHeight();
	else
		return 1;
		*/
}

std::string Text::GetTextString()
{
	return "[" + txt + "]";
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

	bool keepScaleRelative = true;
	bool renderRelative = true;

	if (currentSprite != nullptr)
	{
		renderRelative = currentSprite->keepPositionRelativeToCamera;
		keepScaleRelative = currentSprite->keepScaleRelativeToCamera;
		delete_it(currentSprite->texture);
		delete_it(currentSprite);
	}

	textColor = color; //TODO: Does this even do anything?
	txt = text; // translate the text here
	id = text;

    // empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (txt == "")
		txt = " ";

	SDL_Surface* textSurface = nullptr;
	SDL_Color textColorSDL = { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a };

	for (int i = 0; i < glyphs.size(); i++)
	{
		delete glyphs[i];
	}

	glyphs.clear();

	for (int i = 0; i < txt.size(); i++)
	{
		textSurface = TTF_RenderGlyph_Blended(font, txt[i], textColorSDL);

		if (textSurface != nullptr)
		{
			//TODO: If the texture for this glyph already exists, don't recreate it
			Texture* textTexture = new Texture(&txt[i]);
			textTexture->LoadTexture(textSurface);			

			Sprite* newSprite = new Sprite(textTexture, renderer->shaders[ShaderName::GUI]);
			newSprite->keepScaleRelativeToCamera = keepScaleRelative;
			newSprite->keepPositionRelativeToCamera = renderRelative;
			newSprite->filename = txt[i];

			Glyph* newGlyph = new Glyph;
			newGlyph->sprite = newSprite;

			glyphs.push_back(newGlyph);

			// When GetSprite() is called, get the first glyph in the text
			if (i == 0)
				currentSprite = newSprite;

			if (textSurface != nullptr)
				SDL_FreeSurface(textSurface);
		}
	}

	SetPosition(position.x, position.y);

	/*
	if (wrapWidth > 0)
	{
		textSurface = TTF_RenderText_Blended_Wrapped(font, txt.c_str(), textColorSDL, wrapWidth);
	}
	else
	{
		textSurface = TTF_RenderText_Blended(font, txt.c_str(), textColorSDL);
	}
	*/	
}

void Text::AddText(char c, Color color)
{
	bool keepScaleRelative = true;
	bool renderRelative = true;

	//id = text;

	SDL_Surface* textSurface = nullptr;
	SDL_Color textColorSDL = { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a };

	textSurface = TTF_RenderGlyph_Blended(font, c, textColorSDL);

	if (textSurface != nullptr)
	{
		//TODO: If the texture for this glyph already exists, don't recreate it
		Texture* textTexture = new Texture(&c);
		textTexture->LoadTexture(textSurface);

		Sprite* newSprite = new Sprite(textTexture, renderer->shaders[ShaderName::GUI]);
		newSprite->keepScaleRelativeToCamera = keepScaleRelative;
		newSprite->keepPositionRelativeToCamera = renderRelative;
		newSprite->filename = c;

		Glyph* newGlyph = new Glyph;
		newGlyph->sprite = newSprite;

		glyphs.push_back(newGlyph);

		if (textSurface != nullptr)
			SDL_FreeSurface(textSurface);
	}

	SetPosition(position.x, position.y);

	/*
	if (wrapWidth > 0)
	{
		textSurface = TTF_RenderText_Blended_Wrapped(font, txt.c_str(), textColorSDL, wrapWidth);
	}
	else
	{
		textSurface = TTF_RenderText_Blended(font, txt.c_str(), textColorSDL);
	}
	*/
}


void Text::Render(Renderer* renderer)
{
	for (int i = 0; i < glyphs.size(); i++)
	{
		if (glyphs[i]->sprite != nullptr)
			glyphs[i]->sprite->Render(glyphs[i]->position, renderer);
		else
			int test = 0;
	}
}

void Text::Render(Renderer* renderer, Vector2 offset)
{
	for (int i = 0; i < glyphs.size(); i++)
	{
		glyphs[i]->sprite->Render(glyphs[i]->position + offset, renderer);
	}
}

void Text::SetScale(Vector2 newScale)
{
	scale = newScale;

	for (int i = 0; i < glyphs.size(); i++)
	{
		glyphs[i]->sprite->SetScale(newScale);
	}

	SetPosition(position.x, position.y);
}

//TODO: Make sure to account for offsetting all the letters
void Text::SetPosition(const float x, const float y)
{
	position.x = x;
	position.y = y;

	Vector2 currentPosition = Vector2(position);

	for (int i = 0; i < glyphs.size(); i++)
	{
		currentPosition.x += glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;
		
		//TODO: Maybe add some space between the letters?

		//TODO: Implement word wrap here
		if (wrapWidth > 0 && currentPosition.x > wrapWidth * 2)
		{
			//TODO: What is the actual height?
			currentPosition.y += glyphs[i]->sprite->frameHeight * glyphs[i]->sprite->scale.y * 2;
			currentPosition.x = x;
		}		

		glyphs[i]->position = currentPosition;
	}	
}

void Text::SetPosition(const int x, const int y)
{
	SetPosition((float)x, (float)y);
}
