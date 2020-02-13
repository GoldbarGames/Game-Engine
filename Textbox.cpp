#include "Textbox.h"
#include "Renderer.h"

Textbox::Textbox(SpriteManager * manager, Renderer * renderer)
{
	//TODO: Replace these with the real fonts
	textFont = TTF_OpenFont("fonts/default.ttf", 24);
	speakerFont = TTF_OpenFont("fonts/default.ttf", 24);

	position = Vector2(1280, 720);
	boxSprite = new Sprite(0, 0, 1, manager, "assets/gui/textbox.png", 
		renderer->shaders["gui"], Vector2(0, 0));
	boxSprite->keepScaleRelativeToCamera = true;
	boxSprite->renderRelativeToCamera = true;

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
	text->SetText(" ", textColor, boxWidth);

	sprites['l'] = nullptr;
	sprites['c'] = nullptr;
	sprites['r'] = nullptr;
}

Textbox::~Textbox()
{

}

void Textbox::UpdateText(std::string newText)
{
	text->SetText(newText, textColor, boxWidth);
	text->GetSprite()->keepScaleRelativeToCamera = true;
	text->GetSprite()->renderRelativeToCamera = true;
	//TODO: If we want to modify the textbox's text shader, do so here
	//text->GetSprite()->SetShader(renderer->shaders["fade-in-out"]);
}

void Textbox::Render(Renderer * renderer, const int& screenWidth, const int& screenHeight)
{
	if (shouldRender)
	{
		int halfScreenWidth = ((screenWidth * 2) / 2);
		int spriteX = 0; // (screenWidth / 5) * 3;
		int spriteY = screenHeight;

		if (sprites['l'] != nullptr)
		{
			spriteX = halfScreenWidth - (halfScreenWidth/2);
			spriteY = (screenHeight * 2) - (sprites['l']->frameHeight);

			Vector2 renderPosition = Vector2(spriteX + renderer->guiCamera.position.x,
				spriteY + renderer->guiCamera.position.y);

			//TODO: Make sure the position is in the center of the screen
			sprites['l']->Render(renderPosition, 0, -1, SDL_FLIP_NONE, renderer, 0);
		}

		if (sprites['c'] != nullptr)
		{
			spriteX = halfScreenWidth; // +(sprites['c']->frameWidth / 2);
			spriteY = (screenHeight * 2) - (sprites['c']->frameHeight);

			Vector2 renderPosition = Vector2(spriteX + renderer->guiCamera.position.x,
				spriteY + renderer->guiCamera.position.y);

			//TODO: Make sure the position is in the center of the screen
			sprites['c']->Render(renderPosition, 0, -1, SDL_FLIP_NONE, renderer, 0);
		}

		if (sprites['r'] != nullptr)
		{
			spriteX = halfScreenWidth + (halfScreenWidth / 2);
			spriteY = (screenHeight * 2) - (sprites['r']->frameHeight);

			Vector2 renderPosition = Vector2(spriteX + renderer->guiCamera.position.x,
				spriteY + renderer->guiCamera.position.y);

			//TODO: Make sure the position is in the center of the screen
			sprites['r']->Render(renderPosition, 0, -1, SDL_FLIP_NONE, renderer, 0);
		}


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

		Vector2 renderPosition = Vector2(position.x + renderer->guiCamera.position.x,
			position.y + renderer->guiCamera.position.y);

		Vector2 cameraPosition = Vector2(renderer->guiCamera.position.x + offset.x,
			renderer->guiCamera.position.y + offset.y);

		//TODO: Make sure the position is in the center of the screen
		boxSprite->Render(renderPosition, 0, -1, SDL_FLIP_NONE, renderer, 0);
		speaker->Render(renderer, cameraPosition);

		if (text != nullptr)
			text->Render(renderer, cameraPosition);
	}	
}