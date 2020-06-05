#include "Textbox.h"
#include "Renderer.h"
#include "Entity.h"

Textbox::Textbox(SpriteManager* m, Renderer* r)
{
	spriteManager = m;
	renderer = r;

	//TODO: Replace these with the real fonts
	//TODO: How to deal with font sizes? Maybe map from string to map<int, TTF*>
	fonts["default"] = TTF_OpenFont("fonts/default.ttf", 24);
	fonts["fontSourceCodePro"] = TTF_OpenFont("fonts/source-code-pro/SourceCodePro-Regular.ttf", 24);
	fonts["fontDejaVuSansMono"] = TTF_OpenFont("fonts/dejavu-sans-mono/DejaVuSansMono.ttf", 24);
	fonts["fontSpaceMono"] = TTF_OpenFont("fonts/space-mono/SpaceMono-Regular.ttf", 24);

	textFont = fonts["fontSourceCodePro"];
	speakerFont = fonts["fontSourceCodePro"];

	position = Vector2(1280, 720);
	
	boxSprite = new Sprite(0, 0, 1, spriteManager, "assets/gui/textbox.png",
		renderer->shaders[ShaderName::GUI], Vector2(0, 0));

	boxSprite->keepScaleRelativeToCamera = true;
	boxSprite->keepPositionRelativeToCamera = true;

	text = new Text(renderer, textFont, "...", true, true);
	speaker = new Text(renderer, speakerFont, "...", true, true);

	text->SetPosition(1080, 1040);
	speaker->SetPosition(235, 985);

	//TODO: Should we create one texture for each alphabet letter and show the ones relevant to the string?
	speaker->SetText(" ");
	text->SetText(" ", text->textColor, boxWidth);
}

Textbox::~Textbox()
{

}

void Textbox::ChangeBoxFont(const std::string& fontName)
{
	//TODO: What about the backlog font?
	//TODO: How to change the font size?
	//TODO: Make another map that takes the filepath as the key and has the short name as the value
	if (fonts.count(fontName) == 1)
	{
		textFont = fonts[fontName];
		text->SetFont(textFont);
	}		
}

void Textbox::ChangeNameFont(const std::string& fontName)
{
	//TODO: What about the backlog font?
	//TODO: How to change the font size?
	//TODO: Make another map that takes the filepath as the key and has the short name as the value
	if (fonts.count(fontName) == 1)
	{
		speakerFont = fonts[fontName];
		speaker->SetFont(speakerFont);
	}		
}

void Textbox::ChangeNameSprite(const std::string& filepath)
{
	if (nameSprite != nullptr)
		delete nameSprite;

	//TODO: Allow for animations by dissecting the filepath name
	nameSprite = new Sprite(0, 0, 1, spriteManager, filepath,
		renderer->shaders[ShaderName::GUI], Vector2(0, 0));
}

void Textbox::ChangeBoxSprite(const std::string& filepath)
{
	if (boxSprite != nullptr)
		delete boxSprite;

	//TODO: Allow for animations by dissecting the filepath name
	boxSprite = new Sprite(0, 0, 1, spriteManager, filepath,
		renderer->shaders[ShaderName::GUI], Vector2(0, 0));
}

void Textbox::UpdateText(const char c, const Color& color)
{
	std::cout << position.x << "," << position.y << std::endl;

	text->wrapWidth = boxWidth;
	text->AddText(c, color);
	const int boxOffsetX = 120;
	const int boxOffsetY = 1070;
	text->SetPosition(boxOffsetX, boxOffsetY);
}

void Textbox::UpdateText(const std::string& newText, const Color& color)
{
	text->wrapWidth = boxWidth;
	text->SetText(newText, color, boxWidth);
	const int boxOffsetX = 120;
	const int boxOffsetY = 1070;
	text->SetPosition(boxOffsetX, boxOffsetY);
	//text->SetText(newText, text->textColor, boxWidth);

	//TODO: If we want to modify the textbox's text shader, do so here
	//text->GetSprite()->SetShader(renderer->shaders["fade-in-out"]);
}

void Textbox::Render(Renderer * renderer, const int& screenWidth, const int& screenHeight)
{
	if (shouldRender && isReading)
	{
		//TODO: Make sure the position is in the center of the screen
		boxSprite->Render(position, renderer);
		speaker->Render(renderer);

		if (text != nullptr)
		{
			text->Render(renderer);
		}
			
	}	
}