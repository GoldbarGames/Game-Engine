#include "MenuButton.h"
#include "Game.h"


// draw 3 rectangles behind the original image
// 1 = horizontal (top left = (x - thickness, y) width = width + 2*thickness, height = same)
// 2 = vertical (top left = (x, y - thickness) width = same, height = height + 2*thickness)
// 3 = edges (top left = (x - thickness/2, y - thickness / 2), width = thickness, height = thickness)

MenuButton::MenuButton(std::string t, std::string filepath, Vector2 pos, Game& game)
{
	text = t;
	image = new Sprite(1, game.spriteManager, filepath, game.renderer, Vector2(0,0));
	//TODO: What if I want to scale the button to a particular width and height independent of the image?

	position = pos;

	int scaledThickness = thickness * SCALE;

	outlineHorizontal.x = image->GetRect()->x - scaledThickness;
	outlineHorizontal.w = image->GetRect()->w + (2* scaledThickness);

	outlineVertical.y = image->GetRect()->y - scaledThickness;
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
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
	SDL_RenderDrawRect(renderer, &outlineHorizontal);
	SDL_RenderDrawRect(renderer, &outlineVertical);
	SDL_RenderDrawRect(renderer, &outlineCorners);

	//TODO: What should the color be here? Dunno if this is right
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	image->Render(position, 0, renderer);
}