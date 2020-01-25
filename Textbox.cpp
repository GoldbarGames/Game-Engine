#include "Textbox.h"
#include "Renderer.h"

Textbox::Textbox(SpriteManager * manager, Renderer * renderer)
{
	//TODO: Replace these with the real fonts
	textFont = TTF_OpenFont("fonts/default.ttf", 24);
	speakerFont = TTF_OpenFont("fonts/default.ttf", 24);

	position = Vector2(1280, 720);
	boxSprite = new Sprite(0, 0, 1, manager, "assets/gui/textbox.png", 
		renderer->shaders["textbox-default"], Vector2(0, 0));
	boxSprite->keepScaleRelativeToCamera = true;

	text = new Text(renderer, textFont);
	speaker = new Text(renderer, speakerFont);

	if (text->GetSprite() != nullptr)
		text->GetSprite()->keepScaleRelativeToCamera = true;

	if (speaker->GetSprite() != nullptr)
		speaker->GetSprite()->keepScaleRelativeToCamera = true;
	
	text->SetPosition(150, 720 + 320);
	speaker->SetPosition(640, 480);

	//TODO: Should we create one texture for each alphabet letter and show the ones relevant to the string?
	speaker->SetText(" ");
	text->SetTextWrapped(" ", boxWidth);
}

Textbox::~Textbox()
{

}

void Textbox::Render(Renderer * renderer)
{
	if (shouldRender)
	{
		//TODO: X-alignment is not really correct
		// The only way to align it horizontally is to draw the texture differently
		// This code is just so that it does not go outside the bounds of the textbox
		// and is drawn in an order that looks good

		string alignmentX = "LEFT";
		string alignmentY = "TOP";

		if (alignmentX == "LEFT")
		{
			offset.x = text->GetTextWidth();

		}
		else if (alignmentX == "RIGHT")
		{
			offset.x = -1 * text->GetTextWidth();
		}
		else if (alignmentX == "CENTER")
		{
			offset.x = 0;
		}

		if (alignmentY == "TOP")
		{
			offset.y = text->GetTextHeight();
		}
		else if (alignmentY == "BOTTOM")
		{
			offset.y = -1 * text->GetTextHeight();
		}
		else if (alignmentY == "CENTER")
		{
			offset.y = 0;
		}

		Vector2 renderPosition = Vector2(position.x + renderer->camera.position.x,
			position.y + renderer->camera.position.y);

		Vector2 cameraPosition = Vector2(renderer->camera.position.x + offset.x,
			renderer->camera.position.y + offset.y);

		//TODO: Make sure the position is in the center of the screen
		boxSprite->Render(renderPosition, 0, -1, SDL_FLIP_NONE, renderer, 0);
		speaker->Render(renderer, cameraPosition);
		text->Render(renderer, cameraPosition);
	}	
}
