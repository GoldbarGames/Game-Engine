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

	SDL_Surface* textSurface = nullptr;
	SDL_Color textColorSDL = { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a };

	for (int i = 0; i < glyphs.size(); i++)
	{
		delete glyphs[i];
	}

	glyphs.clear();

    // empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (txt == "")
	{
		txt = " ";
	}

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
	alignX = AlignmentX::RIGHT;

	switch (alignX)
	{
	case AlignmentX::LEFT:
		position.x = x;
		break;
	case AlignmentX::CENTER:
		position.x = x + GetTextWidth();
		break;
	case AlignmentX::RIGHT:
		position.x = x + (wrapWidth * 2);
		break;
	default:
		break;
	}

	//TODO: We would use a boxHeight if we had one to calculate these correctly
	switch (alignY)
	{
	case AlignmentY::TOP:
		position.y = y; //text->GetTextHeight();
		break;
	case AlignmentY::CENTER:
		position.y = GetTextHeight();
		break;
	case AlignmentY::BOTTOM:
		position.y = -1 * y;
		break;
	default:
		break;
	}

	Vector2 currentPosition = Vector2(position);

	for (int i = 0; i < glyphs.size(); i++)
	{
		//TODO: Maybe add some space between the letters?
		// sente/nce is

		bool passedWrapLine = false;

		switch (alignX)
		{
		case AlignmentX::LEFT:
			currentPosition.x += glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;
			passedWrapLine = (currentPosition.x > wrapWidth * 2);
			break;
		case AlignmentX::CENTER:
			passedWrapLine = (currentPosition.x > wrapWidth * 2);
			break;
		case AlignmentX::RIGHT:
			currentPosition.x -= glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;
			passedWrapLine = (currentPosition.x < x);
			break;
		default:
			break;
		}

		// Handle word wrap when the most recent letter exceeds the wrap width
		if (wrapWidth > 0 && passedWrapLine)
		{
			// Don't indent new lines when the words wrap past it
			if (glyphs[i]->sprite->filename != " ") 
			{
				// Get the index of the start of the most recent word
				unsigned int wordStartIndex = i;
				for (int k = i; i > 0; k--)
				{
					if (glyphs[k]->sprite->filename == " ")
					{
						wordStartIndex = k+1;
						break;
					}
				}

				// Go through each letter in the most recent word
				// and re-calculate its position based on the wrapping
				float newLineX = 0;
				
				//TODO: What is the actual height?
				float newLineY = currentPosition.y + glyphs[wordStartIndex]->sprite->frameHeight * glyphs[wordStartIndex]->sprite->scale.y * 2;

				//TODO: When we call SetText("") it makes the first glyph a space, so the first line is indented
				// Therefore for each subsequent line, we need to indent by one letter as well.
				// Not sure how to fix this, deal with it later.
				
				switch (alignX)
				{
				case AlignmentX::LEFT:
					newLineX = x + glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
					newLineX += glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
					break;
				case AlignmentX::CENTER:
					newLineX = x + glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
					newLineX += glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
					break;
				case AlignmentX::RIGHT:
					newLineX = x + (wrapWidth * 2) - glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
					newLineX += glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
					break;
				default:
					break;
				}

				for (int k = wordStartIndex; k < glyphs.size(); k++)
				{
					if (glyphs[k]->sprite->filename != " ")
					{
						glyphs[k]->position.x = newLineX;
						glyphs[k]->position.y = newLineY;

						currentPosition.x = newLineX;
						currentPosition.y = newLineY;

						switch (alignX)
						{
						case AlignmentX::LEFT:
							newLineX += glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
							break;
						case AlignmentX::CENTER:
							newLineX += glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
							break;
						case AlignmentX::RIGHT:
							newLineX -= glyphs[wordStartIndex]->sprite->frameWidth * glyphs[wordStartIndex]->sprite->scale.x * 2;
							break;
						default:
							break;
						}
						
						i = k;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				// When we see the empty space, we know that we have reached the end of the new word
				// (unless we don't see any empty space)
			}
		}		

		glyphs[i]->position = currentPosition;
	}	
}

void Text::SetPosition(const int x, const int y)
{
	SetPosition((float)x, (float)y);
}
