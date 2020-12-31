#include "Text.h"
#include "Renderer.h"
#include "Sprite.h"
#include "FontInfo.h"
#include "Animator.h"

Text::Text() : Entity(Vector2(0,0))
{
	etype = "text";
}

Text::~Text()
{
	for (int i = 0; i < glyphs.size(); i++)
	{
		delete_it(glyphs[i]);
	}

	glyphs.clear();
}

void Text::SetColor(Color newColor)
{
	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			glyphs[i]->sprite.color = newColor;
		}
	}
	else
	{
		currentSprite.color = newColor;
	}
}

int Text::GetTextWidth() 
{ 
	int width = 1;

	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			width += glyphs[i]->sprite.texture->GetWidth();
		}
	}
	else if (currentSprite.texture != nullptr)
	{		
		return currentSprite.texture->GetWidth();
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
			width += glyphs[i]->sprite.texture->GetWidth();
			if (width > wrapWidth)
			{
				width = 0;
				height += glyphs[i]->sprite.texture->GetHeight();
			}
			else if (wrapWidth == 0)
			{
				return glyphs[0]->sprite.texture->GetHeight();
			}
		}

		if (glyphs.size() > 0)
		{
			if (glyphs[0]->sprite.texture != nullptr)
			{
				return glyphs[0]->sprite.texture->GetHeight();
			}
		}
	}
	else if (currentSprite.texture != nullptr)
	{
		return currentSprite.texture->GetHeight();
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

Glyph* Text::GetLastGlyph()
{
	if (isRichText && glyphs.size() > 0)
	{
		return glyphs[glyphs.size() - 1];
	}
	else
	{
		return nullptr;
	}
}

std::string Text::GetTextString()
{
	return "[" + txt + "]";
}

//TODO: Refactor these constructors a little bit
Text::Text(FontInfo* newFontInfo) : Entity(Vector2(0,0))
{
	currentFontInfo = newFontInfo;
	font = currentFontInfo->GetRegularFont();
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
}

Text::Text(FontInfo* newFontInfo, const std::string& txt, bool relPos, bool relScale) : Entity(Vector2(0, 0))
{
	currentFontInfo = newFontInfo;
	font = currentFontInfo->GetRegularFont();
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
	SetText(txt);

	currentSprite.keepPositionRelativeToCamera = relPos;
	currentSprite.keepScaleRelativeToCamera = relScale;
}

Text::Text(FontInfo* newFontInfo, const std::string& txt, Color color) : Entity(Vector2(0, 0))
{
	currentFontInfo = newFontInfo;
	font = currentFontInfo->GetRegularFont();
	position.x = 0;
	position.y = 0;
	SetPosition(0, 0);
	SetText(txt, color);
}



void Text::SetFont(TTF_Font* newFont)
{
	font = newFont;
}

//TODO: Maybe modify this or make another function to pass in a shader?
void Text::SetText(const std::string& text, Color color, Uint32 wrapWidth)
{
	// don't do anything if it would result in the same thing
	if (txt == text && currentSprite.color == color)
		return;

	if (!isRichText)
	{
		SetTextAsOneSprite(text, color, wrapWidth);
		return;
	}

	bool keepScaleRelative = true;
	bool renderRelative = true;

	renderRelative = currentSprite.keepPositionRelativeToCamera;
	keepScaleRelative = currentSprite.keepScaleRelativeToCamera;
	//delete_it(currentSprite->texture);
	//delete_it(currentSprite);

	// We don't actually need to delete the glyph's sprite's texture.
	// The Sprite Manager handles this when the game is closed.	
	for (int i = 0; i < glyphs.size(); i++)
	{
		if (glyphs[i] != nullptr)
			delete_it(glyphs[i]);
	}

	glyphs.clear();

	txt = text; // translate the text here
	id = text;

    // empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (txt == "")
	{
		txt = " ";
	}

	for (int i = 0; i < txt.size(); i++)
	{
		Texture* textTexture = GetTexture(font, txt[i], currentFontInfo->GetFontSize());

		if (textTexture != nullptr)
		{
			Glyph* newGlyph = neww Glyph;
			newGlyph->sprite = Sprite(textTexture, Renderer::GetTextShader());
			newGlyph->sprite.keepScaleRelativeToCamera = keepScaleRelative;
			newGlyph->sprite.keepPositionRelativeToCamera = renderRelative;
			newGlyph->sprite.color = { color.b, color.g, color.r, color.a };
			newGlyph->letter = txt[i];
			newGlyph->scale = currentScale;

			if (txt[i] == '\n')
			{
				newGlyph->sprite.color.a = 0;
			}

			glyphs.push_back(newGlyph);

			// When GetSprite() is called, get the first glyph in the text
			if (i == 0)
			{
				currentSprite = newGlyph->sprite;
			}				
		}
	}

	SetPosition(position.x, position.y);
}

Texture* Text::GetTexture(TTF_Font* f, char c, int size)
{	
	return Animator::spriteManager->GetTexture(f, c, size);
}

void Text::AddImage(Sprite* newSprite)
{
	bool keepScaleRelative = true;
	bool renderRelative = true;

	if (newSprite != nullptr)
	{
		newSprite->keepScaleRelativeToCamera = keepScaleRelative;
		newSprite->keepPositionRelativeToCamera = renderRelative;
		
		Glyph* newGlyph = neww Glyph;
		newGlyph->scale = currentScale;
		newGlyph->sprite = Sprite(newSprite->texture, newSprite->shader);
		delete_it(newSprite);

		if (glyphs.size() > 0)
		{
			Sprite* previousSprite = &glyphs[glyphs.size() - 1]->sprite;

			float oldRatioX = (previousSprite->frameWidth * glyphs[glyphs.size() - 1]->scale.x);
			float oldRatioY = (previousSprite->frameHeight * glyphs[glyphs.size() - 1]->scale.y);

			float newRatioX = (newSprite->frameWidth * newGlyph->scale.x);
			float newRatioY = (newSprite->frameHeight * newGlyph->scale.y);

			float scaleX = oldRatioX / (float)newRatioX;
			float scaleY = oldRatioY / (float)newRatioY;

			newGlyph->scale = Vector2(scaleY, scaleY);
		}

		glyphs.push_back(newGlyph);
	}

	SetPosition(position.x, position.y);
}

void Text::AddText(char c, Color color)
{
	bool keepScaleRelative = true;
	bool renderRelative = true;

	Texture* textTexture = GetTexture(font, c, currentFontInfo->GetFontSize());

	if (textTexture != nullptr)
	{
		Glyph* newGlyph = neww Glyph;
		newGlyph->sprite.SetTexture(textTexture);
		newGlyph->sprite.SetShader(Renderer::GetTextShader());
		newGlyph->sprite.keepScaleRelativeToCamera = keepScaleRelative;
		newGlyph->sprite.keepPositionRelativeToCamera = renderRelative;
		newGlyph->sprite.color = { color.b, color.g, color.r, color.a };
		newGlyph->letter = c;
		newGlyph->scale = currentScale;

		if (c == '\n')
		{
			newGlyph->sprite.color.a = 0;
		}

		glyphs.push_back(newGlyph);
		txt += c;
	}

	SetPosition(position.x, position.y);
}

// TODO: If the text sprite is also a clickable button,
// then the original color of the text is getting overwritten
// to the color white instead of whatever color it should be.

void Text::SetTextAsOneSprite(const std::string& text, Color color, Uint32 wrapWidth)
{
	// For some reason, it is necessary to flip the color from RGBA to BGRA for drawing text
	Color flippedColor = { color.b, color.g, color.r, color.a };

	// don't do anything if it would result in the same thing
	if (txt == text && currentSprite.color == flippedColor)
		return;

	bool renderRelative = currentSprite.keepPositionRelativeToCamera;
	bool keepScaleRelative = currentSprite.keepScaleRelativeToCamera;

	txt = text; // translate the text here
	id = text;

	// empty string generates a null pointer
	// so a blank space guarantees that the surface pointer will not be null
	if (txt == "")
		txt = " ";

	Texture* textTexture = Animator::spriteManager->GetTexture(font, txt, wrapWidth);
	if (textTexture != nullptr)
	{
		currentSprite.SetTexture(textTexture);
		currentSprite.SetShader(Renderer::GetTextShader());
		currentSprite.color = flippedColor;
		//std::cout << currentSprite.texture << " Creating text " << txt << std::endl;
		currentSprite.keepScaleRelativeToCamera = keepScaleRelative;
		currentSprite.keepPositionRelativeToCamera = renderRelative;
	}
	else
	{
		std::cout << "ERROR loading Text texture" << std::endl;
	}
}


void Text::Render(const Renderer& renderer)
{
	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			if (glyphs[i]->animator != nullptr)
			{
				glyphs[i]->sprite.Render(glyphs[i]->position,
					glyphs[i]->animator->GetSpeed(), renderer, glyphs[i]->scale, rotation);
			}
			else
			{
				glyphs[i]->sprite.Render(glyphs[i]->position, 0, renderer, glyphs[i]->scale, rotation);
			}
		}
	}
	else
	{
		currentSprite.Render(position, renderer, scale, rotation);
	}

}

void Text::Render(const Renderer& renderer, Vector2 offset)
{
	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			glyphs[i]->sprite.Render(glyphs[i]->position + offset, renderer, glyphs[i]->scale);
		}
	}
	else
	{
		currentSprite.Render(position + offset, renderer, scale);
	}
}

void Text::SetScale(Vector2 newScale)
{
	scale = newScale;

	if (isRichText)
	{
		for (int i = 0; i < glyphs.size(); i++)
		{
			glyphs[i]->scale = newScale;
		}
	}
	else
	{
		//SetScale(scale);
	}

	SetPosition(position.x, position.y);
}

//TODO: Make sure to account for offsetting all the letters
void Text::SetPosition(const float x, const float y)
{
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

	float wrapX = x;
	float glyphHeight = 0;
	int index = 0;

	int numberOfLines = 0;
	
	// Keep track of the last index in the glyph array that goes into each line
	lineNumToIndex.clear();

	// First, split the text into multiple lines, then align each line
	for (int i = 0; i < glyphs.size(); i++)
	{
		//TODO: Maybe add some space between the letters?
		int width = glyphs[i]->sprite.frameWidth * glyphs[i]->scale.x * Camera::MULTIPLIER;

		if (i == 0)
		{
			glyphHeight = glyphs[i]->sprite.frameHeight * glyphs[i]->scale.y * Camera::MULTIPLIER;
		}

		// The only reason to keep track of the current line is to get the total size
		// and the indices of the most recently added word
		wrapX += width;

		// Handle word wrap when the most recent letter exceeds the wrap width
		if (glyphs[i]->letter == '\n' || (wrapWidth > 0 && (wrapX > wrapWidth * Camera::MULTIPLIER)))
		{
			if (glyphs[i]->letter != ' ')
			{
				int endOfLineIndex = i;
				int newX = x;

				// In order to place the entire word on the next line,
				// we go backward to find the space before the first letter
				if (glyphs[i]->letter != '\n')
				{
					while (glyphs[endOfLineIndex]->letter != ' ')
					{
						//std::cout << glyphs[endOfLineIndex]->sprite.GetFileName() << std::endl;
						endOfLineIndex--;

						// This sets the new line's start x based on the width of the word on the new line
						newX += glyphs[endOfLineIndex]->sprite.frameWidth * glyphs[endOfLineIndex]->scale.x * Camera::MULTIPLIER;
						
						if (endOfLineIndex < 0)
							break;
					}
				}

				// make a record of which glyph index should be the end of this line
				lineNumToIndex[numberOfLines] = endOfLineIndex;
				//std::cout << "Line " << numberOfLines << ": " << endOfLineIndex << std::endl;
							
				wrapX = newX;
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
			currentPosition.x += glyphs[i]->sprite.frameWidth * glyphs[i]->scale.x * Camera::MULTIPLIER;
			glyphs[i]->position = currentPosition;

			if (i > 0 && i == lineNumToIndex[lineNumber])
			{
				lineNumber++;
				currentPosition.x = position.x;

				if (glyphs[i]->letter != '\n')
				{
					currentPosition.x += glyphs[i]->sprite.frameWidth * glyphs[i]->scale.x * Camera::MULTIPLIER;
				}
				else // TODO: Not sure if this will always work properly in the future
				{
					// This fixes an indentation issue with line breaks
					currentPosition.x += glyphs[i]->sprite.frameWidth * glyphs[i]->scale.x;
				}

				currentPosition.y += glyphs[i]->sprite.frameHeight * glyphs[i]->scale.y * Camera::MULTIPLIER;
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
				lineWidth += glyphs[i]->sprite.frameWidth * glyphs[i]->scale.x * Camera::MULTIPLIER;

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
				currentPosition.x += glyphs[i]->sprite.frameWidth * glyphs[i]->scale.x * Camera::MULTIPLIER;

				glyphs[i]->position = currentPosition;
				glyphs[i]->position.x -= lineWidths[lineNumber] / Camera::MULTIPLIER;

				if (i > 0 && i == lineNumToIndex[lineNumber])
				{
					lineNumber++;
					lineWidth = 0;
					if (wrapWidth == 0)
						currentPosition.x = position.x;
					else
						currentPosition.x = wrapWidth;
					currentPosition.y += glyphs[i]->sprite.frameHeight * glyphs[i]->scale.y * Camera::MULTIPLIER;
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
			currentPosition.x -= glyphs[i]->sprite.frameWidth * glyphs[i]->scale.x * Camera::MULTIPLIER;
			glyphs[i]->position = currentPosition;

			int lineIndexFromEnd = lineNumToIndex.size() - lineNumber - 1;
			if (i > 0 && i == lineNumToIndex[lineIndexFromEnd])
			{
				lineNumber++;
				currentPosition.x = position.x + (wrapWidth * 2);
				currentPosition.y -= glyphs[i]->sprite.frameHeight * glyphs[i]->scale.y * Camera::MULTIPLIER;
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
