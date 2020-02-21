#include "Textbox.h"
#include "Renderer.h"
#include "Entity.h"

Textbox::Textbox(SpriteManager * manager, Renderer * renderer)
{
	//TODO: Replace these with the real fonts
	textFont = TTF_OpenFont("fonts/default.ttf", 24);
	speakerFont = TTF_OpenFont("fonts/default.ttf", 24);

	position = Vector2(1280, 720);
	boxSprite = new Sprite(0, 0, 1, manager, "assets/gui/textbox.png", 
		renderer->shaders["gui"], Vector2(0, 0));
	boxSprite->keepScaleRelativeToCamera = true;
	boxSprite->keepPositionRelativeToCamera = true;

	text = new Text(renderer, textFont, "...", true, true);
	speaker = new Text(renderer, speakerFont, "...", true, true);

	text->SetPosition(1080, 1040);
	speaker->SetPosition(700, 960);

	//TODO: Should we create one texture for each alphabet letter and show the ones relevant to the string?
	speaker->SetText(" ");
	text->SetText(" ", text->textColor, boxWidth);
}

Textbox::~Textbox()
{

}

void Textbox::UpdateText(std::string newText)
{
	text->SetText(newText, text->textColor, boxWidth);

	//TODO: If we want to modify the textbox's text shader, do so here
	//text->GetSprite()->SetShader(renderer->shaders["fade-in-out"]);
}

void Textbox::Render(Renderer * renderer, const int& screenWidth, const int& screenHeight)
{
	if (shouldRender)
	{
		string alignmentX = "LEFT";
		string alignmentY = "TOP";

		const int boxOffsetX = 1350;
		const int boxOffsetY = 1060;

		if (alignmentX == "LEFT")
		{
			offset.x = boxOffsetX - boxWidth + text->GetTextWidth();
		}
		else if (alignmentX == "RIGHT")
		{
			offset.x = boxOffsetX + boxWidth - text->GetTextWidth();
		}
		else if (alignmentX == "CENTER")
		{
			offset.x = boxOffsetX + text->GetTextWidth();
		}

		//TODO: We would use a boxHeight if we had one to calculate these correctly
		if (alignmentY == "TOP")
		{
			offset.y = boxOffsetY; //text->GetTextHeight();
		}
		else if (alignmentY == "BOTTOM")
		{
			offset.y = -1 * boxOffsetY;
		}
		else if (alignmentY == "CENTER")
		{
			offset.y = text->GetTextHeight();
		}

		Vector2 renderPosition = Vector2(position.x + renderer->guiCamera.position.x,
			position.y + renderer->guiCamera.position.y);

		Vector2 cameraPosition = Vector2(renderer->guiCamera.position.x + offset.x,
			renderer->guiCamera.position.y + offset.y);

		//TODO: Make sure the position is in the center of the screen
		boxSprite->Render(position, renderer);
		speaker->Render(renderer);

		if (text != nullptr)
		{
			text->SetPosition(offset.x, offset.y);
			text->Render(renderer);
		}
			
	}	
}