#include "Textbox.h"
#include "Renderer.h"
#include "Entity.h"
#include "Game.h"

Textbox::Textbox(SpriteManager& m, Renderer& r)
{
	spriteManager = &m;
	renderer = &r;

	//TODO: Allow for this font to be defined via a file at startup
	fontInfoText = renderer->game->CreateFont("SazanamiGothic", 24);
	fontInfoSpeaker = renderer->game->CreateFont("SazanamiGothic", 24);

	// These values can all be changed from the cutscene definition file at startup,
	// although they will give errors here if they cannot find the images

	boxObject = neww Entity(glm::vec3(1280, 720, 0));

	boxObject->GetSprite()->SetTexture(spriteManager->GetImage("assets/gui/textbox1.png"));
	boxObject->GetSprite()->SetShader(renderer->shaders[ShaderName::GUI]);
	boxObject->GetSprite()->keepScaleRelativeToCamera = true;
	boxObject->GetSprite()->keepPositionRelativeToCamera = true;

	nameObject = neww Entity(glm::vec3(1280, 720, 0));

	nameObject->GetSprite()->SetTexture(spriteManager->GetImage("assets/gui/namebox1.png"));
	nameObject->GetSprite()->SetShader(renderer->shaders[ShaderName::GUI]);
	nameObject->GetSprite()->keepScaleRelativeToCamera = true;
	nameObject->GetSprite()->keepPositionRelativeToCamera = true;

	text = neww Text(fontInfoText, "...", true, true);
	speaker = neww Text(fontInfoSpeaker, "...", true, true);

	//TODO: Customize these things from a file as well
	text->SetPosition(1080, 1040);
	speaker->SetPosition(235, 985);
	text->SetScale(Vector2(0.25f, 0.25f));
	boxWidth = 1130;

	text->isRichText = true;
	speaker->isRichText = false;

	clickToContinue = neww Entity(glm::vec3(0,0,0));

	std::vector<AnimState*> animStates = spriteManager->ReadAnimData("data/animators/cursor/cursor.animations");
	Animator* newAnimator = neww Animator("cursor/cursor", animStates, "samepage");
	newAnimator->SetBool("endOfPage", false);
	
	clickToContinue->SetAnimator(*newAnimator);
	clickToContinue->SetScale(Vector2(0.5f, 0.5f));
	clickToContinue->GetSprite()->keepPositionRelativeToCamera = true;
	clickToContinue->GetSprite()->keepScaleRelativeToCamera = true;

	speaker->SetText(" ");
	text->SetText(" ", text->GetSprite()->color, boxWidth);

	shouldRender = false;
}

Textbox::~Textbox()
{
	if (clickToContinue != nullptr)
		delete_it(clickToContinue);

	if (text != nullptr)
		delete_it(text);

	if (speaker != nullptr)
		delete_it(speaker);

	if (boxObject != nullptr)
		delete_it(boxObject);

	if (nameObject != nullptr)
		delete_it(nameObject);
}

void Textbox::SetFontSize(int newSize)
{
	float newValue = newSize / (float)fontInfoText->GetFontSize();
	text->currentScale = Vector2(newValue, newValue);
}

void Textbox::SetCursorPosition(bool endOfPage)
{
	SetCursorPosition(endOfPage, text->GetLastGlyphPosition());
}

// This sets the position either to the end of the text line,
// or to the end of the text line in the current backlog line
void Textbox::SetCursorPosition(bool endOfPage, glm::vec3 newCursorPos)
{
	clickToContinue->GetAnimator()->SetBool("endOfPage", endOfPage);
	clickToContinue->GetAnimator()->Update(*clickToContinue);
	clickToContinue->GetAnimator()->DoState(*clickToContinue);
	clickToContinue->SetScale(Vector2(0.5f, 0.5f));

	newCursorPos.x += clickToContinue->GetSprite()->frameWidth;
	clickToContinue->SetPosition(newCursorPos);
}

//TODO: What about a way to change the backlog font?

void Textbox::ChangeBoxFont(const std::string& fontName, const int size)
{
	fontInfoText = renderer->game->CreateFont(fontName, size);
	if (fontInfoText != nullptr)
	{
		text->SetFont(fontInfoText->GetRegularFont());
		text->currentFontInfo = fontInfoText;
	}
}

void Textbox::ChangeNameFont(const std::string& fontName, const int size)
{
	fontInfoSpeaker = renderer->game->CreateFont(fontName, size);
	if (fontInfoSpeaker != nullptr)
	{
		speaker->SetFont(fontInfoSpeaker->GetRegularFont());
		speaker->currentFontInfo = fontInfoSpeaker;
	}	
}

void Textbox::ChangeNameSprite(const std::string& filepath)
{
	if (filepath.size() > 0 && nameObject->GetSprite() != nullptr)
	{
		nameObject->GetSprite()->SetTexture(spriteManager->GetImage(filepath));
	}
}

void Textbox::ChangeBoxSprite(const std::string& filepath)
{
	if (filepath.size() > 0 && boxObject->GetSprite() != nullptr)
	{
		boxObject->GetSprite()->SetTexture(spriteManager->GetImage(filepath));
	}
}

void Textbox::UpdateText(const char c, const Color& color)
{
	text->wrapWidth = boxWidth;
	text->AddText(c, color);
	text->SetPosition(boxOffsetX, boxOffsetY);

	if (useShadow)
	{
		// We need one shadow per line due to line breaks and word wrap
		while (shadows.size() < text->lineNumToIndex.size())
		{
			Text* newShadow = neww Text(fontInfoText, "", true, true);
			newShadow->isRichText = false;
			newShadow->SetColor({ 0, 0, 0, 255 });
			shadows.emplace_back(newShadow);
		}

		int lineNumStart = 0;
		for (int i = 0; i < shadows.size(); i++)
		{
			if (text->lineNumToIndex.count(i) != 0)
			{
				std::string lineText = "";
				int maxIndex = text->lineNumToIndex[i] == 0 ? text->glyphs.size() : text->lineNumToIndex[i];
				for (int k = lineNumStart; k < maxIndex; k++)
				{
					lineText += text->glyphs[k]->letter;
				}

				lineNumStart = text->lineNumToIndex[i];

				shadows[i]->wrapWidth = boxWidth;
				shadows[i]->SetTextAsOneSprite(lineText, { 0, 0, 0, 255 }, boxWidth);
				shadows[i]->SetPosition(boxOffsetX + (shadows[i]->GetSprite()->frameWidth + 19), boxOffsetY + (88 * i));
			}
		}


	}

	clickToContinue->SetPosition(glm::vec3(boxOffsetX, boxOffsetY, 0));
	fullTextString += c;
}

void Textbox::UpdateText(const std::string& newText, const Color& color)
{
	text->wrapWidth = boxWidth;
	text->SetText(newText, color, boxWidth);

	if (useShadow)
	{

		// Doesn't seem to be called

		/*
		if (shadow == nullptr)
		{
			shadow = neww Text(fontInfoText, "", true, true);
			shadow->isRichText = false;
			shadow->SetPosition(text->GetPosition().x, text->GetPosition().y);
			shadow->SetColor({ 0, 0, 0, 255 });
		}

		shadow->wrapWidth = boxWidth;
		shadow->SetText(newText, color, boxWidth);
		*/
	}

	clickToContinue->SetPosition(glm::vec3(boxOffsetX, boxOffsetY, 0));
	fullTextString = newText;

	// If we want to modify the textbox's text shader, do so here
}

void Textbox::Render(const Renderer& renderer, const int& screenWidth, const int& screenHeight)
{
	if (shouldRender && isReading)
	{
		boxObject->Render(renderer);
		
		if (speaker->txt != "" && speaker->txt != " ")
		{
			nameObject->Render(renderer);
		}

		if (text != nullptr)
		{
			if (useShadow && shadows.size() > 0)
			{

				for (int i = 0; i < text->lineNumToIndex.size(); i++)
				{
					Text* shadow = shadows[i];
					glm::vec3 pos = shadow->GetPosition();

					// TODO: Make this more efficient!
					for (int i = 2; i < 5; i += 2)
					{
						shadow->SetPosition(pos.x - i, pos.y);
						shadow->Render(renderer);
						shadow->SetPosition(pos.x, pos.y - i);
						shadow->Render(renderer);
						shadow->SetPosition(pos.x + i, pos.y);
						shadow->Render(renderer);
						shadow->SetPosition(pos.x, pos.y + i);
						shadow->Render(renderer);

						int k = i - 1;

						shadow->SetPosition(pos.x - k, pos.y - k);
						shadow->Render(renderer);
						shadow->SetPosition(pos.x - k, pos.y + k);
						shadow->Render(renderer);
						shadow->SetPosition(pos.x + k, pos.y - k);
						shadow->Render(renderer);
						shadow->SetPosition(pos.x + k, pos.y + k);
						shadow->Render(renderer);
					}

					shadow->SetPosition(pos.x, pos.y);
				}

			
			}

			text->Render(renderer);
		}

		if (speaker->txt != "")
		{
			speaker->Render(renderer);
		}			
	}	
}