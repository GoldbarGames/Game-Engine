#include "Textbox.h"
#include "Renderer.h"
#include "Entity.h"

Textbox::Textbox(SpriteManager& m, Renderer& r)
{
	spriteManager = &m;
	renderer = &r;

	//TODO: Replace these with the real fonts
	//TODO: How to deal with font sizes? Maybe map from string to map<int, TTF*>

	fonts["default"] = neww FontInfo("fonts/default.ttf", 24);

	fonts["SourceCodePro"] = neww FontInfo("fonts/source-code-pro/SourceCodePro-Regular.ttf", 24);

	fonts["DejaVuSansMono"] = neww FontInfo("fonts/dejavu-sans-mono/DejaVuSansMono.ttf", 24);

	fonts["SpaceMono"] = neww FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
	fonts["SpaceMono"]->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
	fonts["SpaceMono"]->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
	fonts["SpaceMono"]->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");

	currentFontInfo = fonts["SpaceMono"];

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

	text->isRichText = true;
	speaker->isRichText = false;

	clickToContinue = neww Entity(Vector2(0,0));

	std::vector<AnimState*> animStates = spriteManager->ReadAnimData("data/animators/cursor/cursor.animations");

	Animator* newAnimator = neww Animator("cursor", animStates, "samepage");
	newAnimator->SetBool("endOfPage", false);
	newAnimator->SetRelativeAllStates(true);
	//newAnimator->SetScaleAllStates(Vector2(0.5f, 0.5f));

	clickToContinue->SetAnimator(*newAnimator);
	clickToContinue->GetSprite()->SetScale(Vector2(0.5f, 0.5f));

	speaker->SetText(" ");
	text->SetText(" ", text->textColor, boxWidth);
}

Textbox::~Textbox()
{
	if (clickToContinue != nullptr)
		delete_it(clickToContinue);

	// TODO: When we try to delete these (text and speaker)
	// they are dangling pointers (trying to delete already deleted data)
	// and should have been set to nullptr some time before

	if (text != nullptr)
		delete_it(text);

	if (speaker != nullptr)
		delete_it(speaker);

	if (boxObject != nullptr)
		delete_it(boxObject);

	if (nameObject != nullptr)
		delete_it(nameObject);

	for (auto& [key, val] : fonts)
	{
		if (val != nullptr)
			delete_it(val);
	}
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
	clickToContinue->GetSprite()->SetScale(Vector2(0.5f, 0.5f));

	newCursorPos.x += clickToContinue->GetSprite()->frameWidth;
	clickToContinue->SetPosition(newCursorPos);
}

void Textbox::ChangeBoxFont(const std::string& fontName)
{
	//TODO: What about the backlog font?
	//TODO: How to change the font size?
	//TODO: Make another map that takes the filepath as the key and has the short name as the value
	if (fonts.count(fontName) == 1)
	{
		currentFontInfo = fonts[fontName];
		text->SetFont(currentFontInfo->GetRegularFont());
	}		
}

void Textbox::ChangeNameFont(const std::string& fontName)
{
	//TODO: What about the backlog font?
	//TODO: How to change the font size?
	//TODO: Make another map that takes the filepath as the key and has the short name as the value
	if (fonts.count(fontName) == 1)
	{
		currentFontInfo = fonts[fontName];
		speaker->SetFont(currentFontInfo->GetRegularFont());
	}		
}

void Textbox::ChangeNameSprite(const std::string& filepath)
{
	if (nameObject->GetSprite() != nullptr)
		delete nameObject->GetSprite();

	//TODO: Allow for animations by dissecting the filepath name
	Sprite* newSprite = neww Sprite(0, 0, 1, *spriteManager, filepath,
		renderer->shaders[ShaderName::GUI], Vector2(0, 0));
	nameObject->SetSprite(*newSprite);

	nameObject->GetSprite()->keepScaleRelativeToCamera = true;
	nameObject->GetSprite()->keepPositionRelativeToCamera = true;
}

void Textbox::ChangeBoxSprite(const std::string& filepath)
{
	if (boxObject->GetSprite() != nullptr)
		delete boxObject->GetSprite();

	//TODO: Allow for animations by dissecting the filepath name
	Sprite* newSprite = neww Sprite(0, 0, 1, *spriteManager, filepath,
		renderer->shaders[ShaderName::GUI], Vector2(0, 0));
	boxObject->SetSprite(*newSprite);

	boxObject->GetSprite()->keepScaleRelativeToCamera = true;
	boxObject->GetSprite()->keepPositionRelativeToCamera = true;
}

void Textbox::UpdateText(const char c, const Color& color)
{
	text->wrapWidth = boxWidth;
	text->AddText(c, color);
	const int boxOffsetX = 120;
	const int boxOffsetY = 1070;
	text->SetPosition(boxOffsetX, boxOffsetY);
	clickToContinue->SetPosition(Vector2(boxOffsetX, boxOffsetY));
}

void Textbox::UpdateText(const std::string& newText, const Color& color)
{
	text->wrapWidth = boxWidth;
	text->SetText(newText, color, boxWidth);
	const int boxOffsetX = 120;
	const int boxOffsetY = 1070;
	text->SetPosition(boxOffsetX, boxOffsetY);
	clickToContinue->SetPosition(Vector2(boxOffsetX, boxOffsetY));
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