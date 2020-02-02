#include "MenuButton.h"
#include "Game.h"

MenuButton::MenuButton(std::string txt, std::string filepath, std::string function, Vector2 pos, Game& game)
{
	image = new Sprite(1, game.spriteManager, filepath, game.renderer->shaders["default"], Vector2(0,0));

	text = new Text(game.renderer, game.theFont);
	text->SetText(txt);
	//text->SetPosition(pos.x, pos.y + (image->GetRect()->h / 2) - (text->GetTextHeight()/2));
	text->SetPosition(pos.x, pos.y - (text->GetTextHeight() / 2));

	name = function;

	//TODO: What if I want to scale the button to a particular width and height independent of the image?
	//ANSWER: See the EditorButton

	position = pos;

	int scaledThickness = thickness;

	image->renderRelativeToCamera = true;
	image->keepScaleRelativeToCamera = true;
	text->GetSprite()->renderRelativeToCamera = true;
	text->GetSprite()->keepScaleRelativeToCamera = true;

	/*

	outlineHorizontal.x = image->GetRect()->x - scaledThickness;
	outlineHorizontal.y = image->GetRect()->y;
	outlineHorizontal.w = image->GetRect()->w + (2* scaledThickness);
	outlineHorizontal.h = image->GetRect()->h;

	outlineVertical.x = image->GetRect()->x;
	outlineVertical.y = image->GetRect()->y - scaledThickness;
	outlineVertical.w = image->GetRect()->w;
	outlineVertical.h = image->GetRect()->h + (2* scaledThickness);

	outlineCorners.x = image->GetRect()->x - (scaledThickness /2);
	outlineCorners.y = image->GetRect()->y - (scaledThickness /2);
	outlineCorners.w = image->GetRect()->w + scaledThickness;
	outlineCorners.h = image->GetRect()->h + scaledThickness;
	*/
}


MenuButton::~MenuButton()
{

}

void MenuButton::Render(Renderer* renderer)
{
	// Draw the outline
	/*
	if (isSelected)
	{
		// TODO: Make this a color type that we can swap out
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255); //yellow
		SDL_RenderFillRect(renderer->renderer, &outlineHorizontal);
		SDL_RenderFillRect(renderer->renderer, &outlineVertical);
		SDL_RenderFillRect(renderer->renderer, &outlineCorners);
	}
	*/

	// Draw the button image
	//TODO: What should the color be here? Dunno if this is right
	//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
	image->Render(position, 0, -1, SDL_FLIP_NONE, renderer, 0);
	text->Render(renderer);
}


BaseButton* MenuButton::Update(Game& game, const Uint8* currentKeyStates)
{
	pressedAnyKey = true;

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		if (buttonPressedUp != nullptr)
		{
			return buttonPressedUp;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		if (buttonPressedDown != nullptr)
		{
			return buttonPressedDown;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		if (buttonPressedLeft != nullptr)
		{
			return buttonPressedLeft;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		if (buttonPressedRight != nullptr)
		{
			return buttonPressedRight;
		}
	}

	pressedAnyKey = false;

	return this;
}

void MenuButton::SetOptionColors(Color color)
{
	text->SetText(text->txt, color);
}