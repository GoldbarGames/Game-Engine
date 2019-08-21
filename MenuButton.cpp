#include "MenuButton.h"
#include "Game.h"

MenuButton::MenuButton(std::string txt, std::string filepath, std::string function, Vector2 pos, Game& game)
{
	image = new Sprite(1, game.spriteManager, filepath, game.renderer, Vector2(0,0));

	text = new Text(game.renderer, game.theFont);
	text->SetText(txt);
	text->SetPosition(pos.x, pos.y + (image->GetRect()->h / 2) - (text->textWindowRect.h/2));

	functionName = function;

	//TODO: What if I want to scale the button to a particular width and height independent of the image?
	//ANSWER: See the EditorButton

	position = pos;
	image->windowRect.x = pos.x;
	image->windowRect.y = pos.y;

	int scaledThickness = thickness * SCALE;

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
}


MenuButton::~MenuButton()
{
}

void MenuButton::Render(SDL_Renderer* renderer)
{
	// Draw the outline
	if (isSelected)
	{
		// TODO: Make this a color type that we can swap out
		SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); //yellow
		SDL_RenderFillRect(renderer, &outlineHorizontal);
		SDL_RenderFillRect(renderer, &outlineVertical);
		SDL_RenderFillRect(renderer, &outlineCorners);
	}

	// Draw the button image
	//TODO: What should the color be here? Dunno if this is right
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	image->Render(position, 0, -1, SDL_FLIP_NONE, renderer);

	text->Render(renderer);
}

void MenuButton::SetButtonsUpDownLeftRight(MenuButton* up, MenuButton* down, MenuButton* left, MenuButton* right)
{
	buttonPressedUp = up;
	buttonPressedDown = down;
	buttonPressedLeft = left;
	buttonPressedRight = right;
}