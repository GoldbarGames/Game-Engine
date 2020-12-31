#include "Textbox.h"
#include "Renderer.h"
#include "Entity.h"
#include "Game.h"

Textbox::Textbox(SpriteManager& m, Renderer& r)
{
	spriteManager = &m;
	renderer = &r;

	//TODO: Replace these with the real fonts
	//TODO: How to deal with font sizes? Maybe map from string to map<int, TTF*>
	boxWidth = 1130;
	currentFontInfo = renderer->game->CreateFont("SazanamiGothic", 24);

	//TODO: Should we have a way to define the starting box position?
	boxObject = neww Entity(Vector2(1280, 720));

	//TODO: Have a way to specify the image for the box
	boxObject->GetSprite()->SetTexture(spriteManager->GetImage("assets/gui/textbox1.png"));
	boxObject->GetSprite()->SetShader(renderer->shaders[ShaderName::GUI]);
	boxObject->GetSprite()->keepScaleRelativeToCamera = true;
	boxObject->GetSprite()->keepPositionRelativeToCamera = true;

	//TODO: Should we have a way to define the starting box position?
	nameObject = neww Entity(Vector2(1280, 720));

	//TODO: Have a way to specify the image for the box
	nameObject->GetSprite()->SetTexture(spriteManager->GetImage("assets/gui/namebox1.png"));
	nameObject->GetSprite()->SetShader(renderer->shaders[ShaderName::GUI]);
	nameObject->GetSprite()->keepScaleRelativeToCamera = true;
	nameObject->GetSprite()->keepPositionRelativeToCamera = true;

	text = neww Text(currentFontInfo, "...", true, true);
	speaker = neww Text(currentFontInfo, "...", true, true);

	text->SetPosition(1080, 1040);
	speaker->SetPosition(235, 985);

	text->SetScale(Vector2(0.25f, 0.25f));

	text->isRichText = true;
	speaker->isRichText = false;

	clickToContinue = neww Entity(Vector2(0,0));

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
	float newValue = newSize / (float)currentFontInfo->GetFontSize();
	text->currentScale = Vector2(newValue, newValue);
}

void Textbox::SetCursorPosition(bool endOfPage)
{
	SetCursorPosition(endOfPage, text->GetLastGlyphPosition());
}

// This sets the position either to the end of the text line,
// or to the end of the text line in the current backlog line
void Textbox::SetCursorPosition(bool endOfPage, Vector2 newCursorPos)
{
	clickToContinue->GetAnimator()->SetBool("endOfPage", endOfPage);
	clickToContinue->GetAnimator()->Update(*clickToContinue);
	clickToContinue->GetAnimator()->DoState(*clickToContinue);
	clickToContinue->SetScale(Vector2(0.5f, 0.5f));

	newCursorPos.x += clickToContinue->GetSprite()->frameWidth;
	clickToContinue->SetPosition(newCursorPos);
}

void Textbox::ChangeBoxFont(const std::string& fontName, const int size)
{
	//TODO: What about the backlog font?
	//TODO: More advanced features?

	currentFontInfo = renderer->game->CreateFont(fontName, size);
	if (currentFontInfo != nullptr)
	{
		text->SetFont(currentFontInfo->GetRegularFont());
		text->currentFontInfo = currentFontInfo;
	}
}

void Textbox::ChangeNameFont(const std::string& fontName, const int size)
{
	//TODO: What about the backlog font?
	//TODO: How to change the font size?
	//TODO: Make another map that takes the filepath as the key and has the short name as the value
	if (renderer->game->fonts.count(fontName) == 1)
	{
		speaker->SetFont(currentFontInfo->GetRegularFont());
		speaker->currentFontInfo = renderer->game->CreateFont(fontName, size);
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
	const int boxOffsetX = 120;
	const int boxOffsetY = 1070;
	text->SetPosition(boxOffsetX, boxOffsetY);
	clickToContinue->SetPosition(Vector2(boxOffsetX, boxOffsetY));
	fullTextString += c;
}

void Textbox::UpdateText(const std::string& newText, const Color& color)
{
	text->wrapWidth = boxWidth;
	text->SetText(newText, color, boxWidth);
	const int boxOffsetX = 120;
	const int boxOffsetY = 1070;
	//text->SetPosition(boxOffsetX, boxOffsetY);
	clickToContinue->SetPosition(Vector2(boxOffsetX, boxOffsetY));
	fullTextString = newText;
	//text->SetText(newText, text->textColor, boxWidth);

	//TODO: If we want to modify the textbox's text shader, do so here
	//text->GetSprite()->SetShader(renderer->shaders["fade-in-out"]);
}

void Textbox::Render(const Renderer& renderer, const int& screenWidth, const int& screenHeight)
{
	if (shouldRender && isReading)
	{
		//TODO: Make sure the position is in the center of the screen
		boxObject->Render(renderer);
		
		if (speaker->txt != "" && speaker->txt != " ")
		{
			nameObject->Render(renderer);
		}

		if (text != nullptr)
		{
			text->Render(renderer);
		}

		if (speaker->txt != "")
		{
			speaker->Render(renderer);
		}			
	}	
}