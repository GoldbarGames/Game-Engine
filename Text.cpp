#include "Text.h"
#include "Renderer.h"
#include "Sprite.h"

int Text::GetTextWidth() 
{ 
	int width = 1;

	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			if (glyphs[i]->sprite != nullptr)
				width += glyphs[i]->sprite->texture->GetWidth();
		}
	}
	else if (currentSprite != nullptr)
	{		
		return currentSprite->texture->GetWidth();
	}
	
	return width;
}

int Text::GetTextHeight() 
{	
	int height = 1;

	if (isRichText)
	{
		int width = 0;
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
	}
	else if (currentSprite != nullptr)
	{
		return currentSprite->texture->GetHeight();
	}

	return height;
}

Vector2 Text::GetLastGlyphPosition()
{
	if (isRichText && glyphs.size() > 0)
	{
		return glyphs[glyphs.size() - 1]->position;
	}

	return position;
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
	for (int i = 0; i < glyphs.size(); i++)
	{
		delete_it(glyphs[i]);
	}

	glyphs.clear();
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

	if (!isRichText)
	{
		SetTextAsOneSprite(text, color, wrapWidth);
		return;
	}

	bool keepScaleRelative = true;
	bool renderRelative = true;

	if (currentSprite != nullptr)
	{
		renderRelative = currentSprite->keepPositionRelativeToCamera;
		keepScaleRelative = currentSprite->keepScaleRelativeToCamera;
		//delete_it(currentSprite->texture);
		//delete_it(currentSprite);

		//TODO: Probably shouldn't have two pointers to the same memory location,
		// since when it gets deleted here, the other point is pointing at deleted memory
		if (glyphs.size() > 0)
			glyphs[0] = nullptr;
	}

	//TODO: There is a memory leak here because we never actually delete the glyph's sprite's texture.
	// But actually, we don't really want to delete it. We want to re-use it for the next time.
	// So make some kind of manager where we can grab that texture.
	for (int i = 0; i < glyphs.size(); i++)
	{
		delete_it(glyphs[i]);
	}

	glyphs.clear();

	textColor = color; //TODO: Does this even do anything?
	txt = text; // translate the text here
	id = text;

	//SDL_Surface* textSurface = nullptr;
	SDL_Color textColorSDL = { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a };

    // empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (txt == "")
	{
		txt = " ";
	}

	for (int i = 0; i < txt.size(); i++)
	{
		Texture* textTexture = GetTexture(font, txt[i], textColorSDL);

		if (textTexture != nullptr)
		{
			Sprite* newSprite = new Sprite(textTexture, renderer->shaders[ShaderName::GUI]);
			newSprite->keepScaleRelativeToCamera = keepScaleRelative;
			newSprite->keepPositionRelativeToCamera = renderRelative;
			newSprite->filename = txt[i];
			newSprite-> SetScale(currentScale);

			Glyph* newGlyph = new Glyph;
			newGlyph->sprite = newSprite;

			glyphs.push_back(newGlyph);

			// When GetSprite() is called, get the first glyph in the text
			if (i == 0)
				currentSprite = newSprite;
		}
	}

	SetPosition(position.x, position.y);
}

Texture* Text::GetTexture(TTF_Font* f, char c, SDL_Color col)
{	
	GlyphSurfaceData data;
	data.fontName = TTF_FontFaceStyleName(f);
	data.glyph = c;
	data.color = col;

	if (glyphTextures[data].get() == nullptr)
	{
		SDL_Surface* textSurface = TTF_RenderGlyph_Blended(f, data.glyph, data.color);
		
		Texture* textTexture = nullptr;
		textTexture = new Texture(&data.glyph);
		textTexture->LoadTexture(textSurface);
		
		glyphTextures[data].reset(textTexture);

		if (textSurface != nullptr)
			SDL_FreeSurface(textSurface);
	}

	return glyphTextures[data].get();
}

void Text::AddText(char c, Color color)
{
	bool keepScaleRelative = true;
	bool renderRelative = true;

	SDL_Color textColorSDL = { (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a };
	Texture* textTexture = GetTexture(font, c, textColorSDL);

	if (textTexture != nullptr)
	{
		Sprite* newSprite = new Sprite(textTexture, renderer->shaders[ShaderName::GUI]);
		newSprite->keepScaleRelativeToCamera = keepScaleRelative;
		newSprite->keepPositionRelativeToCamera = renderRelative;
		newSprite->filename = c;
		newSprite->SetScale(currentScale);

		Glyph* newGlyph = new Glyph;
		newGlyph->sprite = newSprite;

		glyphs.push_back(newGlyph);
	}

	SetPosition(position.x, position.y);
}

void Text::SetTextAsOneSprite(string text, Color color, Uint32 wrapWidth)
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
	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			if (glyphs[i]->sprite != nullptr)
			{
				glyphs[i]->sprite->Render(glyphs[i]->position, renderer);
			}
		}
	}
	else if(currentSprite != nullptr)
	{
		currentSprite->Render(position, renderer);
	}

}

void Text::Render(Renderer* renderer, Vector2 offset)
{
	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			glyphs[i]->sprite->Render(glyphs[i]->position + offset, renderer);
		}
	}
	else if (currentSprite != nullptr)
	{
		currentSprite->Render(position + offset, renderer);
	}
}

void Text::SetScale(Vector2 newScale)
{
	scale = newScale;

	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			glyphs[i]->sprite->SetScale(newScale);
		}
	}
	else if (currentSprite != nullptr)
	{
		currentSprite->SetScale(scale);
	}

	SetPosition(position.x, position.y);
}

//TODO: Make sure to account for offsetting all the letters
void Text::SetPosition(const float x, const float y)
{
	alignX = AlignmentX::LEFT;

	//TODO: Make this work for aligning non-rich text as well
	if (!isRichText)
	{
		switch (alignX)
		{
		case AlignmentX::LEFT:
			position.x = x;
			break;
		case AlignmentX::CENTER:
			position.x = x;
			break;
		case AlignmentX::RIGHT:
			position.x = wrapWidth - GetTextWidth();
			break;
		default:
			break;
		}

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

		position.x = x;
		position.y = y;
		return;
	}

	

	Vector2 currentPosition = Vector2(x, y);

	float wrapX = currentPosition.x;
	float glyphHeight = 0;
	int index = 0;

	int numberOfLines = 0;
	
	// Keep track of the last index in the glyph array that goes into each line
	lineNumToIndex.clear();

	// First, split the text into multiple lines, then align each line
	for (int i = 0; i < glyphs.size(); i++)
	{
		//TODO: Maybe add some space between the letters?
		int width = glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;

		if (i == 0)
		{
			glyphHeight = glyphs[i]->sprite->frameHeight * glyphs[i]->sprite->scale.y * 2;
		}

		// The only reason to keep track of the current line is to get the total size
		// and the indices of the most recently added word
		wrapX += width;

		// Handle word wrap when the most recent letter exceeds the wrap width
		if (wrapWidth > 0 && (wrapX > wrapWidth * 2))
		{
			if (glyphs[i]->sprite->filename != " ")
			{
				int endOfLineIndex = i;
				while (glyphs[endOfLineIndex]->sprite->filename != " ")
				{
					endOfLineIndex--;
					if (endOfLineIndex < 0)
						break;
				}

				// make a record of which glyph index should be the end of this line
				lineNumToIndex[numberOfLines] = endOfLineIndex;
				//std::cout << "Line " << numberOfLines << ": " << endOfLineIndex << std::endl;
				
				wrapX = position.x;
				numberOfLines++;
			}

		}
	}
	
	int lineNumber = 0;
	index = 0;

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


	switch (alignX)
	{
	case AlignmentX::LEFT:

		position.x = x;
		currentPosition = position;

		for (int i = 0; i < glyphs.size(); i++)
		{
			currentPosition.x += glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;
			glyphs[i]->position = currentPosition;

			if (i > 0 && i == lineNumToIndex[lineNumber])
			{
				lineNumber++;
				currentPosition.x = position.x;
				currentPosition.y += glyphs[i]->sprite->frameHeight * glyphs[i]->sprite->scale.y * 2;;
			}
		}

		break;
	case AlignmentX::CENTER:
		{
			position.x = x;

			if (wrapWidth == 0)
				currentPosition.x = position.x;
			else
				currentPosition.x = wrapWidth;


			currentPosition.y = position.y;

			float lineWidth = 0;
			std::unordered_map<int, float> lineWidths;

			// On each line, sum together the width of all glyphs on that line, and subtract half of that from the position
			for (int i = 0; i < glyphs.size(); i++)
			{
				lineWidth += glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;

				if (i > 0 && i == lineNumToIndex[lineNumber])
				{
					lineWidths[lineNumber] = lineWidth;
					lineNumber++;					
					lineWidth = 0;
				}
			}

			lineWidths[lineNumber] = lineWidth;
			lineNumber = 0;

			for (int i = 0; i < glyphs.size(); i++)
			{			
				currentPosition.x += glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;

				glyphs[i]->position = currentPosition;
				glyphs[i]->position.x -= lineWidths[lineNumber] / 2.0f;

				if (i > 0 && i == lineNumToIndex[lineNumber])
				{
					lineNumber++;
					lineWidth = 0;
					if (wrapWidth == 0)
						currentPosition.x = position.x;
					else
						currentPosition.x = wrapWidth;
					currentPosition.y += glyphs[i]->sprite->frameHeight * glyphs[i]->sprite->scale.y * 2;;
				}
			}


		}	

		break;
	case AlignmentX::RIGHT:

		position.x = x;
		currentPosition.x = position.x + (wrapWidth * 2);		
		// Add an offset to all previous lines to align them with the top of the textbox
		currentPosition.y = position.y + (lineNumToIndex.size() * glyphHeight);

		lineNumber = 0;		
		for (int i = glyphs.size() - 1; i >= 0; i--)
		{
			// To avoid having the earlier lines on the bottom, we need to subtract here to make the next lines move up,
			currentPosition.x -= glyphs[i]->sprite->frameWidth * glyphs[i]->sprite->scale.x * 2;
			glyphs[i]->position = currentPosition;

			int lineIndexFromEnd = lineNumToIndex.size() - lineNumber - 1;
			if (i > 0 && i == lineNumToIndex[lineIndexFromEnd])
			{
				lineNumber++;
				currentPosition.x = position.x + (wrapWidth * 2);
				currentPosition.y -= glyphs[i]->sprite->frameHeight * glyphs[i]->sprite->scale.y * 2;
			}
		}

		break;
	default:
		break;
	}


	

		
}

void Text::SetPosition(const int x, const int y)
{
	SetPosition((float)x, (float)y);
}
